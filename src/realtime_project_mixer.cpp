#include "hush/realtime_project_mixer.hpp"

#include <algorithm>
#include <exception>
#include <limits>
#include <unordered_set>
#include <utility>

namespace hush {
namespace {
void set_error(Error& error, const char* code, const std::string& entity,
               const char* message) {
    error = {code, entity, message, "error"};
}
} // namespace

bool RealtimeProjectMixer::configure(const Project& project,
                                     const std::vector<std::string>& track_bus_ids,
                                     const std::string& master_bus_id,
                                     std::size_t max_frames,
                                     Error& error) {
    if (configured_) {
        set_error(error, "project_mixer_already_configured", project.id,
                  "Stop the audio reader and construct a new mixer to change topology");
        return false;
    }
    if (project.tracks.empty() || project.buses.empty() || max_frames == 0 ||
        track_bus_ids.size() != project.tracks.size()) {
        set_error(error, "project_mixer_topology_invalid", project.id,
                  "Tracks, buses, frame capacity, and one explicit bus assignment per track are required");
        return false;
    }
    if (project.tracks.size() > RealtimeTrackBusRouter::kQueueCapacity) {
        set_error(error, "project_mixer_track_limit", project.id,
                  "Initial track controls exceed the bounded realtime command queue capacity");
        return false;
    }

    std::unordered_set<std::string> track_ids;
    track_ids.reserve(project.tracks.size());
    for (const auto& track : project.tracks) {
        if (track.id.empty() || !track_ids.insert(track.id).second) {
            set_error(error, "project_mixer_track_id_invalid", track.id,
                      "Track IDs must be non-empty and unique");
            return false;
        }
    }

    std::vector<std::size_t> mapping;
    mapping.reserve(track_bus_ids.size());
    for (std::size_t i = 0; i < track_bus_ids.size(); ++i) {
        const auto bus = std::find_if(project.buses.begin(), project.buses.end(),
            [&](const Bus& candidate) { return candidate.id == track_bus_ids[i]; });
        if (bus == project.buses.end()) {
            set_error(error, "project_mixer_bus_assignment_invalid", project.tracks[i].id,
                      "Track assignment references a bus ID absent from the project");
            return false;
        }
        mapping.push_back(static_cast<std::size_t>(std::distance(project.buses.begin(), bus)));
    }

    BusGraph graph;
    RoutingError graph_error;
    try {
        if (!graph.configure(project.buses, project.routes, graph_error)) {
            error = {graph_error.code, graph_error.entity_id, graph_error.message, "error"};
            return false;
        }
    } catch (const std::exception&) {
        set_error(error, "project_mixer_graph_allocation_failed", project.id,
                  "Could not construct the project bus graph");
        return false;
    }

    ProcessorSetupError setup_error;
    if (!engine_.replace_graph(graph, max_frames, master_bus_id, setup_error)) {
        error = {setup_error.code, setup_error.entity_id, setup_error.message, "error"};
        return false;
    }

    if (!router_.configure(std::move(mapping), project.buses.size(), max_frames)) {
        set_error(error, "project_mixer_router_setup_failed", project.id,
                  "Could not allocate the preconfigured track-to-bus router");
        return false;
    }

    for (std::size_t i = 0; i < project.tracks.size(); ++i) {
        const auto& track = project.tracks[i];
        if (!router_.enqueue_track_control(i, track.gain, track.pan, track.mute, track.solo)) {
            set_error(error, "project_mixer_initial_control_invalid", track.id,
                      "Track controls are invalid or the bounded command queue rejected initialization");
            return false;
        }
    }

    track_ids_.reserve(project.tracks.size());
    for (const auto& track : project.tracks) track_ids_.push_back(track.id);
    bus_count_ = project.buses.size();
    max_frames_ = max_frames;
    configured_ = true;
    error = {};
    return true;
}

bool RealtimeProjectMixer::enqueue_track_control(const std::string& stable_track_id,
                                                 float gain, float pan,
                                                 bool mute, bool solo) noexcept {
    if (!configured_) return false;
    const auto it = std::find(track_ids_.begin(), track_ids_.end(), stable_track_id);
    if (it == track_ids_.end()) return false;
    const auto index = static_cast<std::size_t>(std::distance(track_ids_.begin(), it));
    return router_.enqueue_track_control(index, gain, pan, mute, solo);
}

RealtimeProcessStatus RealtimeProjectMixer::process(const float* const* mono_tracks,
                                                    std::size_t track_count,
                                                    std::size_t frames,
                                                    float* output_left,
                                                    float* output_right) noexcept {
    if (output_left == nullptr || output_right == nullptr) return RealtimeProcessStatus::NullChannel;
    if (!configured_) {
        std::fill_n(output_left, frames, 0.0F);
        std::fill_n(output_right, frames, 0.0F);
        return RealtimeProcessStatus::NotConfigured;
    }
    if (frames > max_frames_) {
        std::fill_n(output_left, frames, 0.0F);
        std::fill_n(output_right, frames, 0.0F);
        return RealtimeProcessStatus::FrameCountTooLarge;
    }
    if (!router_.process(mono_tracks, track_count, frames)) {
        std::fill_n(output_left, frames, 0.0F);
        std::fill_n(output_right, frames, 0.0F);
        return RealtimeProcessStatus::InvalidInputCount;
    }
    return engine_.process_to_stereo(router_.bus_inputs(), router_.bus_count(),
                                     frames, output_left, output_right);
}

} // namespace hush
