#pragma once

#include "hush/autosave_controller.hpp"

#include <filesystem>

namespace hush {

using ProjectFileWriter = bool (*)(const std::filesystem::path&, const Project&, Error&);

enum class FinalSaveOutcome { NotRequired, Saved, Failed };

// Non-audio shutdown helper. The project file is written before the recovery
// journal; a journal failure after a successful project write is still reported.
FinalSaveOutcome finalize_dirty_project(bool dirty,
                                        const std::filesystem::path& project_path,
                                        const Project& snapshot,
                                        AutosaveController& autosave,
                                        ProjectFileWriter project_writer,
                                        Error& error);

} // namespace hush
