#pragma once

#include "hush/project.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_set>
#include <vector>

namespace hush {
// Assign one identity to clips created by a simultaneous recording pass.
// Control-thread only; validates the entire request before mutating any clip.
[[nodiscard]] inline bool assign_recording_take(Project& project,
    const std::vector<std::string>& clip_ids, Error& error) {
    error = {};
    const auto fail = [&error](const char* code, const std::string& entity, const char* message) {
        error = {code, entity, message, "error"};
        return false;
    };
    if (project.id.empty()) return fail("take_project_id_empty", {}, "Project ID must not be empty.");
    if (clip_ids.empty()) return fail("take_clip_list_empty", {}, "At least one recorded clip is required.");
    std::unordered_set<std::string> requested;
    requested.reserve(clip_ids.size());
    std::vector<std::size_t> selected;
    selected.reserve(clip_ids.size());
    std::uint32_t highest = 0U;
    for (const auto& clip : project.clips) highest = std::max(highest, clip.take_number);
    for (const auto& id : clip_ids) {
        if (id.empty()) return fail("take_clip_id_empty", id, "Clip ID must not be empty.");
        if (!requested.insert(id).second) return fail("take_clip_duplicate", id, "A clip may appear only once.");
        std::size_t index = project.clips.size();
        for (std::size_t i = 0; i < project.clips.size(); ++i)
            if (project.clips[i].id == id) { index = i; break; }
        if (index == project.clips.size()) return fail("take_clip_unknown", id, "Recorded clip does not exist.");
        const auto& clip = project.clips[index];
        if (!clip.take_id.empty() || clip.take_number != 0U)
            return fail("take_clip_already_assigned", id, "Clip already has take identity.");
        selected.push_back(index);
    }
    if (highest == std::numeric_limits<std::uint32_t>::max())
        return fail("take_number_exhausted", {}, "No further take numbers are available.");
    const std::uint32_t next = highest + 1U;
    const std::string next_id = project.id + "/take/" + std::to_string(next);
    for (const auto& clip : project.clips)
        if (clip.take_id == next_id) return fail("take_id_collision", next_id, "Generated take ID already exists.");
    for (const auto index : selected) { project.clips[index].take_id = next_id; project.clips[index].take_number = next; }
    return true;
}
} // namespace hush
