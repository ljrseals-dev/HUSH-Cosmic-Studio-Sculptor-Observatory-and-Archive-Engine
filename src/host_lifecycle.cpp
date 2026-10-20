#include "hush/host_lifecycle.hpp"

namespace hush {

FinalSaveOutcome finalize_dirty_project(bool dirty,
                                        const std::filesystem::path& project_path,
                                        const Project& snapshot,
                                        AutosaveController& autosave,
                                        ProjectFileWriter project_writer,
                                        Error& error) {
    if (!dirty) {
        error = {};
        return FinalSaveOutcome::NotRequired;
    }
    if (project_writer == nullptr) {
        error = {"project_writer_missing", snapshot.id,
                 "No project-file writer was provided for final save.", "error"};
        autosave.report_project_write_failure(error.message);
        return FinalSaveOutcome::Failed;
    }
    if (!project_writer(project_path, snapshot, error)) {
        if (error.message.empty()) {
            error = {"project_write_failed", snapshot.id,
                     "Could not persist project before shutdown.", "error"};
        }
        autosave.report_project_write_failure(error.message);
        return FinalSaveOutcome::Failed;
    }
    if (autosave.save_now(snapshot, error) != AutosaveController::Outcome::Saved) {
        return FinalSaveOutcome::Failed;
    }
    error = {};
    return FinalSaveOutcome::Saved;
}

} // namespace hush
