#pragma once

#include "hush/autosave_controller.hpp"

#include <filesystem>

namespace hush {

using ProjectFileWriter = bool (*)(const std::filesystem::path&, const Project&, Error&);

enum class FinalSaveOutcome {
    NotRequired,
    Saved,
    Failed
};

// Runs only from the host's non-audio shutdown path. The project writer and
// journal controller are injectable so failure paths can be tested without ALSA.
// The canonical project file is written before the recovery journal. If the file
// write succeeds but the journal append fails, the project may be current on disk,
// but the overall result is Failed and the caller must surface the failure.
FinalSaveOutcome finalize_dirty_project(bool dirty,
                                        const std::filesystem::path& project_path,
                                        const Project& snapshot,
                                        AutosaveController& autosave,
                                        ProjectFileWriter project_writer,
                                        Error& error);

} // namespace hush
