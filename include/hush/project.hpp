#pragma once
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
};
struct Project {
    std::uint32_t schema_version{1};
    std::string id{"project-1"};
    PanLaw pan_law{PanLaw::EqualPowerMinus3dB};
    std::vector<Track> tracks;
    bool operator==(const Project&) const = default;
};
std::string to_json(const Project& project);
bool from_json(const std::string& json, Project& out, Error& error);
}
