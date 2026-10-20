#pragma once

namespace hush {

// The user-facing decision for replacing or closing a project with unsaved edits.
enum class ProjectCloseChoice { Save, Discard, Cancel };

// A Save choice only permits closing if the save actually completed. A cancelled
// file dialog or a failed write must leave the current project open and dirty.
[[nodiscard]] constexpr bool should_close_project(bool dirty,
                                                   ProjectCloseChoice choice,
                                                   bool save_succeeded) noexcept {
    if (!dirty) return true;
    if (choice == ProjectCloseChoice::Discard) return true;
    if (choice == ProjectCloseChoice::Save) return save_succeeded;
    return false;
}

} // namespace hush
