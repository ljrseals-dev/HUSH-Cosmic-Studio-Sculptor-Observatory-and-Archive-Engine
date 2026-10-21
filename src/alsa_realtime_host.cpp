#include "hush/alsa_audio_backend.hpp"
#include "hush/autosave_controller.hpp"
#include "hush/bus_graph.hpp"
#include "hush/project.hpp"
#include "hush/realtime_bus_engine_host.hpp"
#include "hush/realtime_bus_engine_host_audio_adapter.hpp"

#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <thread>

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
        << "Usage: hush_alsa_realtime_host [--help] [--project FILE] [--journal FILE]\n"
        << "  --project FILE  Load a local schema-v1 project before audio starts.\n"
        << "  --journal FILE  Recovery journal path (default: FILE.hush-recovery.journal).\n"
        << "Exit codes: 0 normal shutdown; 1 argument, project-open, graph, or audio startup failure;\n"
        << "            2 recovery or project/journal persistence failure; 3 terminal ALSA worker failure.\n"
        << "The host currently has no project-edit command surface; loaded project state is not live routing.\n";
}
} // namespace

int main(int argc, char** argv) {
    using namespace hush;

    std::filesystem::path project_path;
    std::filesystem::path journal_path;
    bool has_project = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_help();
            return 0;
        }
        if ((arg == "--project" || arg == "--journal") && i + 1 >= argc) {
            std::cerr << "HUSH_ARGUMENT_ERROR missing value for " << arg << '\n';
            return 1;
        }
        if (arg == "--project") {
            project_path = argv[++i];
            has_project = true;
        } else if (arg == "--journal") {
            journal_path = argv[++i];
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

    RoutingError routing_error;
    BusGraph graph;
    if (!graph.configure(
            {{"capture", "Capture"}, {"master", "Master"}},
            {{"capture", "master", SendTap::PostFader, 1.0F}},
            routing_error)) {
        std::cerr << "Bus graph setup failed: " << routing_error.code
                  << ": " << routing_error.message << '\n';
        return 1;
    }

    constexpr std::size_t max_frames = 4096;
    RealtimeBusEngineHost engine_host;
    ProcessorSetupError setup_error;
    if (!engine_host.replace_graph(graph, max_frames, "master", setup_error)) {
        std::cerr << "Realtime engine setup failed: " << setup_error.code
                  << ": " << setup_error.message << '\n';
        return 1;
    }

    RealtimeBusEngineHostAudioAdapter adapter;
    std::string adapter_error;
    if (!adapter.configure(engine_host, 0, graph.buses().size(), max_frames, adapter_error)) {
        std::cerr << "Audio adapter setup failed: " << adapter_error << '\n';
        return 1;
    }

    AlsaAudioBackend backend;
    AlsaAudioBackend::Config config;
    config.sample_rate = 48000;
    config.period_frames = 128;
    std::string backend_error;
    if (!backend.start(config, &RealtimeBusEngineHostAudioAdapter::alsa_callback,
                      &adapter, backend_error)) {
        std::cerr << "ALSA startup failed: " << backend_error << '\n';
        return 1;
    }

    std::cout << "HUSH ALSA realtime host running at "
              << backend.actual_sample_rate() << " Hz, "
              << backend.actual_period_frames() << " frames/period.\n"
              << "Capture is routed to the Master bus. Press Ctrl-C to stop.\n";
    if (has_project) {
        std::cout << "Loaded project metadata: " << project.id
                  << " (project state is not connected to live routing).\n";
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
                engine_host.reclaim_retired();
                return 2;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    backend.stop();
    engine_host.reclaim_retired();
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
