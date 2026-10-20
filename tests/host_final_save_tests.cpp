#include "hush/host_lifecycle.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
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
    out << hush::to_json(project);
    out.flush();
    if (!out) {
        error = {"project_write_failed", project.id, "cannot write test project", "error"};
        return false;
    }
    error = {};
    return true;
}

bool fail_journal_writer(const std::filesystem::path&, const hush::Project& project,
                         hush::Error& error) {
    error = {"journal_write_failed", project.id, "injected final journal write failure", "error"};
    return false;
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

    AutosaveController no_dirty_controller{RecoveryJournal(journal_path)};
    assert(finalize_dirty_project(false, project_path, project, no_dirty_controller,
                                  &fail_project_writer, error) == FinalSaveOutcome::NotRequired);
    assert(error.message.empty());
    assert(no_dirty_controller.last_save_error() == SaveError::None);

    AutosaveController project_failure_controller{RecoveryJournal(journal_path)};
    assert(finalize_dirty_project(true, project_path, project, project_failure_controller,
                                  &fail_project_writer, error) == FinalSaveOutcome::Failed);
    assert(project_failure_controller.last_save_error() == SaveError::ProjectWriteFailed);
    assert(project_failure_controller.last_save_message() == "injected final project write failure");
    assert(!std::filesystem::exists(project_path));

    AutosaveController journal_failure_controller{RecoveryJournal(journal_path, &fail_journal_writer)};
    assert(finalize_dirty_project(true, project_path, project, journal_failure_controller,
                                  &write_project_writer, error) == FinalSaveOutcome::Failed);
    assert(journal_failure_controller.last_save_error() == SaveError::JournalWriteFailed);
    assert(std::filesystem::exists(project_path));

    std::filesystem::remove(project_path, ignored);
    std::filesystem::remove(journal_path, ignored);
    AutosaveController success_controller{RecoveryJournal(journal_path)};
    assert(finalize_dirty_project(true, project_path, project, success_controller,
                                  &write_project_writer, error) == FinalSaveOutcome::Saved);
    assert(error.message.empty());
    assert(success_controller.last_save_error() == SaveError::None);
    assert(std::filesystem::exists(project_path));
    std::filesystem::remove(project_path, ignored);
    std::filesystem::remove(journal_path, ignored);
}
