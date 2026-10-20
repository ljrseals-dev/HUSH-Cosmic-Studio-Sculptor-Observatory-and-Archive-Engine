#pragma once
#include "hush/bus_graph.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace hush {
struct Error {
    std::string code;
    std::string entity_id;
    std::string message;
    std::string severity{"error"};
};
enum class PanLaw { EqualPowerMinus3dB, LegacyMinus4_5dB };
struct Track {
    std::string id;
    std::string name;
    float gain{1.0F};
    float pan{0.0F};
    bool mute{false};
    bool solo{false};
    bool operator==(const Track&) const = default;
};

// Project-relative path to an imported audio asset. Importing/decoding is a
// control-thread operation; the model stores metadata, not the decoded buffer.
struct MediaSource {
    std::string id;
    std::string relative_path;
    std::uint32_t sample_rate{0};
    std::uint16_t channels{0};
    std::uint64_t frame_count{0};
    bool operator==(const MediaSource&) const = default;
};

// Sample-accurate clip placement. All positions and lengths are integer frames
// in their respective source/project timelines; no floating-point time rounding.
struct AudioClip {
    std::string id;
    std::string track_id;
    std::string source_id;
    std::uint64_t timeline_start_frame{0};
    std::uint64_t source_start_frame{0};
    std::uint64_t frame_count{0};
    float gain{1.0F};
    bool operator==(const AudioClip&) const = default;
};
struct ProjectTempoPoint {
    double beat{0.0};
    double bpm{120.0};
    bool operator==(const ProjectTempoPoint&) const = default;
};

enum class ProjectAutomationInterpolation { Step, Linear };
struct ProjectAutomationPoint {
    double beat{0.0};
    float value{0.0F};
    bool operator==(const ProjectAutomationPoint&) const = default;
};
struct ProjectAutomationLane {
    std::string id;
    std::string target;
    ProjectAutomationInterpolation interpolation{ProjectAutomationInterpolation::Linear};
    std::vector<ProjectAutomationPoint> points;
    bool operator==(const ProjectAutomationLane&) const = default;
};

struct Project {
    std::uint32_t schema_version{1};
    std::string id{"project-1"};
    PanLaw pan_law{PanLaw::EqualPowerMinus3dB};
    std::vector<Track> tracks;
    // Bus routing is part of canonical project state. Validate it with BusGraph before rendering.
    std::vector<Bus> buses;
    std::vector<BusRoute> routes;
    // Optional additive schema-v1 arrays. Older projects without them remain valid.
    std::vector<MediaSource> media_sources;
    std::vector<AudioClip> clips;
    // Additive schema-v1 fields. Empty tempo_map means the default 120 BPM at beat zero.
    std::vector<ProjectTempoPoint> tempo_map;
    std::vector<ProjectAutomationLane> automation_lanes;
    bool operator==(const Project&) const = default;
};
std::string to_json(const Project& project);
bool from_json(const std::string& json, Project& out, Error& error);
}
