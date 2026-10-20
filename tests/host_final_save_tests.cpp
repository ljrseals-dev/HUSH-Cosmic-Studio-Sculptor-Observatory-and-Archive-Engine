#include "hush/host_lifecycle.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace {
int journal_writer_calls = 0;

bool fail_project_writer(const std::filesystem::path&, const hush::Project& project,
                         hush::Error& error) {
    error = {"project_write_failed", project.id, "injected final project write failure", "error"};
    return false;
}

bool write_project_writer(const std::filesystem::path& path, const hush::Project& project,
                          hush::Error& error) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        error = {"project_write_failed", project.id, "cannot open test project path", "error"};
        return false;
    }
    const auto json = hush::to_json(project);
    out.write(json.data(), static_cast<std::streamsize>(json.size()));
    out.flush();
    if (!out) {
        error = {"project_write_failed", project.id, "cannot write test project", "error"};
        return false;
    }
    out.close();
    if (out.fail()) {
        error = {"project_write_failed", project.id, "cannot close test project", "error"};
        return false;
    }
    error = {};
    return true;
}

bool fail_journal_writer(const std::filesystem::path&, const hush::Project& project,
                         hush::Error& error) {
    ++journal_writer_calls;
    error = {"journal_write_failed", project.id, "injected final journal write failure", "error"};
    return false;
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

int main() {
    using namespace hush;
    const auto root = std::filesystem::temp_directory_path();
    const auto project_path = root / "hush-final-save-contract.json";
    const auto journal_path = root / "hush-final-save-contract.journal";
    std::error_code ignored;
    std::filesystem::remove(project_path, ignored);
    std::filesystem::remove(journal_path, ignored);

    Project project;
    project.id = "final-save-contract";
    project.tracks.push_back({"track-1", "Lead", 0.75F, 0.0F, false, false});
    Error error;

    // A clean shutdown must not invoke either writer.
    AutosaveController no_dirty_controller{RecoveryJournal(journal_path)};
    assert(finalize_dirty_project(false, project_path, project, no_dirty_controller,
                                  &fail_project_writer, error) == FinalSaveOutcome::NotRequired);
    assert(error.message.empty());
    assert(no_dirty_controller.last_save_error() == SaveError::None);
    assert(!std::filesystem::exists(project_path));

    // A missing writer is a reported failure, not a crash or a silent clean exit.
    AutosaveController missing_writer_controller{RecoveryJournal(journal_path)};
    assert(finalize_dirty_project(true, project_path, project, missing_writer_controller,
                                  nullptr, error) == FinalSaveOutcome::Failed);
    assert(error.code == "project_writer_missing");
    assert(missing_writer_controller.last_save_error() == SaveError::ProjectWriteFailed);
    assert(!std::filesystem::exists(project_path));

    // If the project file fails, the journal writer must not be attempted.
    journal_writer_calls = 0;
    AutosaveController project_failure_controller{RecoveryJournal(journal_path, &fail_journal_writer)};
    assert(finalize_dirty_project(true, project_path, project, project_failure_controller,
                                  &fail_project_writer, error) == FinalSaveOutcome::Failed);
    assert(project_failure_controller.last_save_error() == SaveError::ProjectWriteFailed);
    assert(project_failure_controller.last_save_message() == "injected final project write failure");
    assert(!std::filesystem::exists(project_path));
    assert(journal_writer_calls == 0);

    // A project-file save followed by journal failure is a partial success:
    // the file remains valid, but shutdown must still report failure.
    AutosaveController journal_failure_controller{RecoveryJournal(journal_path, &fail_journal_writer)};
    assert(finalize_dirty_project(true, project_path, project, journal_failure_controller,
                                  &write_project_writer, error) == FinalSaveOutcome::Failed);
    assert(journal_failure_controller.last_save_error() == SaveError::JournalWriteFailed);
    assert(journal_failure_controller.last_save_message() == "injected final journal write failure");
    Project disk_project;
    std::string parse_error;
    assert(from_json(read_file(project_path), disk_project, parse_error));
    assert(disk_project == project);
    assert(journal_writer_calls == 1);

    // Full success requires both the canonical project file and recovery journal.
    std::filesystem::remove(project_path, ignored);
    std::filesystem::remove(journal_path, ignored);
    AutosaveController success_controller{RecoveryJournal(journal_path)};
    assert(finalize_dirty_project(true, project_path, project, success_controller,
                                  &write_project_writer, error) == FinalSaveOutcome::Saved);
    assert(error.message.empty());
    assert(success_controller.last_save_error() == SaveError::None);
    assert(std::filesystem::exists(project_path));
    Project saved_project;
    assert(from_json(read_file(project_path), saved_project, parse_error));
    assert(saved_project == project);
    Project journal_project;
    assert(RecoveryJournal(journal_path).recover_latest(journal_project, error));
    assert(journal_project == project);

    std::filesystem::remove(project_path, ignored);
    std::filesystem::remove(journal_path, ignored);
}
