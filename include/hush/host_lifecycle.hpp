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
FinalSaveOutcome finalize_dirty_project(bool dirty,
                                        const std::filesystem::path& project_path,
                                        const Project& snapshot,
                                        AutosaveController& autosave,
                                        ProjectFileWriter project_writer,
                                        Error& error);

} // namespace hush
