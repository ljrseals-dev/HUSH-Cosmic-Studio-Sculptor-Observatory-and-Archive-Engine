#pragma once
#include "hush/project.hpp"
#include <cstddef>
#include <vector>
namespace hush {
struct StereoBuffer { std::vector<float> left; std::vector<float> right; };
StereoBuffer mix_tracks(const Project& project,
                        const std::vector<std::vector<float>>& mono_tracks);
}
