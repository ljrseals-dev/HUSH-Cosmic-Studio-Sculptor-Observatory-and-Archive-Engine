#include "hush/host_lifecycle.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {
int journal_writer_calls = 0;

bool fail_project_writer(const std::filesystem::path&, const hush::Project& project,
                         hush::Error& error) {
    error = {"project_write_failed", project.id, "injected project write failure", "error"};
    return false;
}

bool throw_project_writer(const std::filesystem::path&, const hush::Project&,
                          hush::Error&) {
    throw std::runtime_error("injected writer exception");
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
    error = {"journal_write_failed", project.id, "injected journal write failure", "error"};
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

    // Clean shutdown does not invoke either writer.
    AutosaveController clean_controller{RecoveryJournal(journal_path)};
    assert(finalize_dirty_project(false, project_path, project, clean_controller,
                                  &fail_project_writer, error) == FinalSaveOutcome::NotRequired);
    assert(error.message.empty() && clean_controller.last_save_error() == SaveError::None);
    assert(!std::filesystem::exists(project_path));

    // Missing and throwing writers become reported failures rather than escaping shutdown.
    AutosaveController missing_controller{RecoveryJournal(journal_path)};
    assert(finalize_dirty_project(true, project_path, project, missing_controller,
                                  nullptr, error) == FinalSaveOutcome::Failed);
    assert(error.code == "project_writer_missing");
    assert(missing_controller.last_save_error() == SaveError::ProjectWriteFailed);

    AutosaveController throwing_controller{RecoveryJournal(journal_path)};
    assert(finalize_dirty_project(true, project_path, project, throwing_controller,
                                  &throw_project_writer, error) == FinalSaveOutcome::Failed);
    assert(error.code == "project_write_exception");
    assert(error.message == "injected writer exception");
    assert(throwing_controller.last_save_error() == SaveError::ProjectWriteFailed);

    // Project-file failure must prevent a journal append.
    journal_writer_calls = 0;
    AutosaveController project_failure{RecoveryJournal(journal_path, &fail_journal_writer)};
    assert(finalize_dirty_project(true, project_path, project, project_failure,
                                  &fail_project_writer, error) == FinalSaveOutcome::Failed);
    assert(error.code == "project_write_failed");
    assert(project_failure.last_save_error() == SaveError::ProjectWriteFailed);
    assert(journal_writer_calls == 0 && !std::filesystem::exists(project_path));

    // If the project file succeeds but journal append fails, report partial success.
    AutosaveController journal_failure{RecoveryJournal(journal_path, &fail_journal_writer)};
    assert(finalize_dirty_project(true, project_path, project, journal_failure,
                                  &write_project_writer, error) == FinalSaveOutcome::Failed);
    assert(journal_failure.last_save_error() == SaveError::JournalWriteFailed);
    assert(journal_writer_calls == 1);
    Project disk_project;
    assert(from_json(read_file(project_path), disk_project, error));
    assert(disk_project == project);

    // Full success requires both the canonical project file and the recovery journal.
    std::filesystem::remove(project_path, ignored);
    std::filesystem::remove(journal_path, ignored);
    AutosaveController success{RecoveryJournal(journal_path)};
    assert(finalize_dirty_project(true, project_path, project, success,
                                  &write_project_writer, error) == FinalSaveOutcome::Saved);
    assert(error.message.empty() && success.last_save_error() == SaveError::None);
    Project saved_project;
    assert(from_json(read_file(project_path), saved_project, error));
    assert(saved_project == project);
    Project journal_project;
    assert(RecoveryJournal(journal_path).recover_latest(journal_project, error));
    assert(journal_project == project);

    std::filesystem::remove(project_path, ignored);
    std::filesystem::remove(journal_path, ignored);
    return 0;
}
