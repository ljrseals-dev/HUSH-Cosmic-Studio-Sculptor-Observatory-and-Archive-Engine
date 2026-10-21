#include "hush/alsa_audio_backend.hpp"
#include "hush/autosave_controller.hpp"
#include "hush/project.hpp"
#include "hush/realtime_project_audio_pipeline.hpp"
#include "hush/recording_session.hpp"

#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
volatile std::sig_atomic_t keep_running = 1;

void handle_signal(int) {
    keep_running = 0;
}

bool write_project_file(const std::filesystem::path& path, const hush::Project& project,
                        hush::Error& error) {
    const std::string json = hush::to_json(project);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        error = {"project_write_failed", project.id,
                 "Could not open project file for writing: " + path.string(), "error"};
        return false;
    }
    file.write(json.data(), static_cast<std::streamsize>(json.size()));
    file.flush();
    if (!file) {
        error = {"project_write_failed", project.id,
                 "Could not write complete project file: " + path.string(), "error"};
        return false;
    }
    file.close();
    if (file.fail()) {
        error = {"project_write_failed", project.id,
                 "Could not close project file cleanly: " + path.string(), "error"};
        return false;
    }
    error = {};
    return true;
}

void print_help() {
    std::cout
        << "HUSH ALSA realtime host\n"
        << "Usage: hush_alsa_realtime_host [--help] [--project FILE] [--journal FILE] [--record FILE]\n"
        << "  --project FILE  Load a local schema-v1 project before audio starts.\n"
        << "  --journal FILE  Recovery journal path (default: FILE.hush-recovery.journal).\n"
        << "  --record FILE   Explicitly record stereo ALSA input to a float WAV on a disk-writer thread.\n"
        << "Exit codes: 0 normal shutdown; 1 argument, project-open, graph, or audio startup failure;\n"
        << "            2 recovery/recording persistence failure; 3 terminal ALSA worker failure.\n"
        << "Project track controls and bus routing are active; tracks beyond capture L/R have no source yet.\n";
}
} // namespace

int main(int argc, char** argv) {
    using namespace hush;

    std::filesystem::path project_path;
    std::filesystem::path journal_path;
    std::filesystem::path recording_path;
    bool has_project = false;
    bool has_recording = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_help();
            return 0;
        }
        if ((arg == "--project" || arg == "--journal" || arg == "--record") && i + 1 >= argc) {
            std::cerr << "HUSH_ARGUMENT_ERROR missing value for " << arg << '\n';
            return 1;
        }
        if (arg == "--project") {
            project_path = argv[++i];
            has_project = true;
        } else if (arg == "--journal") {
            journal_path = argv[++i];
        } else if (arg == "--record") {
            recording_path = argv[++i];
            has_recording = true;
        } else {
            std::cerr << "HUSH_ARGUMENT_ERROR unknown argument: " << arg << '\n';
            return 1;
        }
    }
    if (!has_project && !journal_path.empty()) {
        std::cerr << "HUSH_ARGUMENT_ERROR --journal requires --project\n";
        return 1;
    }
    if (has_project && project_path.empty()) {
        std::cerr << "HUSH_ARGUMENT_ERROR --project path is empty\n";
        return 1;
    }
    if (has_project && journal_path.empty()) {
        journal_path = std::filesystem::path(project_path.string() + ".hush-recovery.journal");
    }

    Project project;
    std::unique_ptr<AutosaveController> autosave;
    if (has_project) {
        std::ifstream input(project_path, std::ios::binary);
        if (!input) {
            std::cerr << "HUSH_PROJECT_OPEN_FAILED path=" << project_path.string() << '\n';
            return 1;
        }
        const std::string json{std::istreambuf_iterator<char>(input),
                               std::istreambuf_iterator<char>()};
        if (input.bad()) {
            std::cerr << "HUSH_PROJECT_READ_FAILED path=" << project_path.string() << '\n';
            return 1;
        }
        Error project_error;
        if (!from_json(json, project, project_error)) {
            std::cerr << "HUSH_PROJECT_INVALID path=" << project_path.string()
                      << " code=" << project_error.code
                      << " message=" << project_error.message << '\n';
            return 1;
        }

        autosave = std::make_unique<AutosaveController>(RecoveryJournal(journal_path));
        std::error_code filesystem_error;
        const bool journal_exists = std::filesystem::exists(journal_path, filesystem_error);
        if (filesystem_error) {
            std::cerr << "HUSH_RECOVERY_CHECK_FAILED path=" << journal_path.string()
                      << " message=" << filesystem_error.message() << '\n';
            return 2;
        }
        if (journal_exists) {
            Error recovery_error;
            bool recovery_applied = false;
            if (!RecoveryJournal(journal_path).recover_if_different(
                    project, recovery_applied, recovery_error)) {
                std::cerr << "HUSH_RECOVERY_FAILED path=" << journal_path.string()
                          << " code=" << recovery_error.code
                          << " message=" << recovery_error.message << '\n';
                return 2;
            }
            if (recovery_applied) {
                if (!write_project_file(project_path, project, project_error)) {
                    std::cerr << "HUSH_RECOVERY_PROJECT_WRITE_FAILED path=" << project_path.string()
                              << " message=" << project_error.message << '\n';
                    return 2;
                }
                if (autosave->save_now(project, recovery_error) != AutosaveController::Outcome::Saved) {
                    std::cerr << "HUSH_RECOVERY_JOURNAL_WRITE_FAILED path=" << journal_path.string()
                              << " message=" << autosave->last_save_message() << '\n';
                    return 2;
                }
                std::cerr << "HUSH_RECOVERY_APPLIED path=" << journal_path.string() << '\n';
            }
        }
    }

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    // When launched without a project, provide two explicit capture source tracks.
    // A loaded project with no tracks is rejected rather than silently inventing tracks.
    Project runtime_project = project;
    if (!has_project) {
        runtime_project.tracks = {
            {"track-1", "Capture L", 1.0F, -1.0F, false, false},
            {"track-2", "Capture R", 1.0F, 1.0F, false, false}
        };
    }

    constexpr std::size_t max_frames = 4096;
    RealtimeProjectAudioPipeline pipeline;
    std::string pipeline_error;
    if (!pipeline.configure(runtime_project, max_frames, pipeline_error)) {
        std::cerr << "Realtime project pipeline setup failed: " << pipeline_error << '\n';
        return 1;
    }

    AlsaAudioBackend backend;
    AlsaAudioBackend::Config config;
    config.sample_rate = 48000;
    config.period_frames = 128;
    std::unique_ptr<RecordingSession> recording;
    if (has_recording) {
        recording = std::make_unique<RecordingSession>();
        std::string recording_error;
        // Five seconds of bounded capture buffering; overflow is counted, never blocked.
        if (!recording->start(recording_path, config.sample_rate, 2U,
                              static_cast<std::size_t>(config.sample_rate) * 5U,
                              2048U, &recording_error)) {
            std::cerr << "HUSH_RECORDING_START_FAILED path=" << recording_path.string()
                      << " message=" << recording_error << '\n';
            return 2;
        }
        pipeline.set_recording_session(recording.get());
    }

    std::string backend_error;
    if (!backend.start(config, &RealtimeProjectAudioPipeline::alsa_callback,
                      &pipeline, backend_error)) {
        if (recording) {
            std::string recording_error;
            if (!recording->stop(&recording_error))
                std::cerr << "HUSH_RECORDING_FINALIZE_FAILED message=" << recording_error << '\n';
            pipeline.set_recording_session(nullptr);
        }
        std::cerr << "ALSA startup failed: " << backend_error << '\n';
        return 1;
    }

    std::cout << "HUSH ALSA realtime host running at "
              << backend.actual_sample_rate() << " Hz, "
              << backend.actual_period_frames() << " frames/period.\n"
              << "Capture L/R feed source tracks 1-2 through track controls and project bus routing.\n"
              << "Additional tracks are silent until a source is connected. Press Ctrl-C to stop.\n";
    if (has_project) {
        std::cout << "Loaded project: " << project.id
                  << " (persisted track bus assignments and gain/pan/mute/solo are active).\n";
    }

    while (keep_running != 0 && backend.running()) {
        if (autosave) {
            Error autosave_error;
            const auto outcome = autosave->poll(
                project, AutosaveController::Clock::now(), backend.running(), autosave_error);
            if (outcome == AutosaveController::Outcome::Failed) {
                std::cerr << "HUSH_AUTOSAVE_FAILED path=" << journal_path.string()
                          << " code=" << autosave_error.code
                          << " message=" << autosave->last_save_message() << '\n';
                backend.stop();
                if (recording) {
                    std::string recording_error;
                    if (!recording->stop(&recording_error))
                        std::cerr << "HUSH_RECORDING_FINALIZE_FAILED message=" << recording_error << '\n';
                    pipeline.set_recording_session(nullptr);
                }
                pipeline.reclaim_retired();
                return 2;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    bool autosave_failed = false;
    if (autosave) {
        Error autosave_error;
        const auto outcome = autosave->poll(
            project, AutosaveController::Clock::now(), false, autosave_error);
        if (outcome == AutosaveController::Outcome::Failed) {
            autosave_failed = true;
            std::cerr << "HUSH_AUTOSAVE_FAILED path=" << journal_path.string()
                      << " code=" << autosave_error.code
                      << " message=" << autosave->last_save_message() << '\n';
        }
    }

    backend.stop();
    bool recording_failed = false;
    if (recording) {
        std::string recording_error;
        if (!recording->stop(&recording_error)) {
            recording_failed = true;
            std::cerr << "HUSH_RECORDING_FINALIZE_FAILED path=" << recording_path.string()
                      << " message=" << recording_error << '\n';
        }
        pipeline.set_recording_session(nullptr);
        std::cout << "Recording frames: " << recording->frames_written()
                  << ", dropped frames: " << recording->dropped_frames()
                  << ", file: " << recording_path.string() << '\n';
    }
    pipeline.reclaim_retired();
    if (autosave_failed || recording_failed) return 2;
    std::cout << "Stopped. Callbacks: " << backend.callback_count()
              << ", observed recoveries/short transfers: "
              << backend.xrun_count() << '\n';
    switch (backend.worker_failure()) {
    case AlsaAudioBackend::WorkerFailure::None:
        return 0;
    case AlsaAudioBackend::WorkerFailure::CaptureRecoveryFailed:
        std::cerr << "HUSH_ALSA_WORKER_FAILED side=capture recover_error="
                  << backend.worker_error_code() << '\n';
        return 3;
    case AlsaAudioBackend::WorkerFailure::PlaybackRecoveryFailed:
        std::cerr << "HUSH_ALSA_WORKER_FAILED side=playback recover_error="
                  << backend.worker_error_code() << '\n';
        return 3;
    }
    std::cerr << "HUSH_ALSA_WORKER_FAILED side=unknown\n";
    return 3;
}
