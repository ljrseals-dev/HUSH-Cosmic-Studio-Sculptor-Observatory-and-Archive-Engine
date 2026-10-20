#pragma once
#include "hush/project.hpp"
#include <cstddef>
#include <vector>

namespace hush {
struct StereoBuffer { std::vector<float> left; std::vector<float> right; };

struct PanGains {
    float left{0.70710678118F};
    float right{0.70710678118F};
};

// Pure control/render helper. Input pan is clamped to [-1, 1]. Both laws are
// continuous at center and have unity gain at the hard-panned side.
[[nodiscard]] PanGains pan_gains(float pan, PanLaw law) noexcept;

StereoBuffer mix_tracks(const Project& project,
                        const std::vector<std::vector<float>>& mono_tracks);
}
