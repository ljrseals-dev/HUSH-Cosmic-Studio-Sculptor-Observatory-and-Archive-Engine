#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hush {

enum class AutomationCurve : std::uint8_t {
    Hold = 0,
    Linear = 1
};

struct AutomationPoint {
    std::uint64_t position_samples{};
    float value{};
    AutomationCurve curve{AutomationCurve::Linear};
};

struct AutomationLane {
    std::string track_id;
    std::string parameter_id;
    std::vector<AutomationPoint> points;
};

// Control-thread-owned editable lane. Build/edit/sort the lane away from the
// audio callback, then publish an immutable snapshot before realtime playback.
// evaluate() itself does not allocate or perform I/O.
class AutomationLaneView {
public:
    explicit AutomationLaneView(const AutomationLane& lane) noexcept
        : lane_(&lane) {}

    [[nodiscard]] float evaluate(std::uint64_t position_samples,
                                 float fallback = 0.0F) const noexcept {
        const auto& points = lane_->points;
        if (points.empty()) return fallback;
        if (position_samples <= points.front().position_samples)
            return points.front().value;

        const auto upper = std::upper_bound(
            points.begin(), points.end(), position_samples,
            [](std::uint64_t position, const AutomationPoint& point) {
                return position < point.position_samples;
            });
        if (upper == points.end()) return points.back().value;

        const auto& left = *(upper - 1);
        const auto& right = *upper;
        if (left.curve == AutomationCurve::Hold ||
            right.position_samples <= left.position_samples) {
            return left.value;
        }

        const auto span = right.position_samples - left.position_samples;
        const auto offset = position_samples - left.position_samples;
        const float t = static_cast<float>(static_cast<double>(offset) /
                                           static_cast<double>(span));
        return left.value + (right.value - left.value) * t;
    }

private:
    const AutomationLane* lane_;
};

// Validate and sort on a non-realtime thread before publishing a lane.
// Duplicate sample positions are rejected to make interpolation deterministic.
[[nodiscard]] inline bool normalize_automation_lane(AutomationLane& lane) {
    if (lane.track_id.empty() || lane.parameter_id.empty()) return false;
    for (const auto& point : lane.points) {
        if (!std::isfinite(point.value)) return false;
        if (point.curve != AutomationCurve::Hold &&
            point.curve != AutomationCurve::Linear) return false;
    }
    std::sort(lane.points.begin(), lane.points.end(),
              [](const AutomationPoint& a, const AutomationPoint& b) {
                  return a.position_samples < b.position_samples;
              });
    for (std::size_t i = 1; i < lane.points.size(); ++i) {
        if (lane.points[i - 1].position_samples ==
            lane.points[i].position_samples) return false;
    }
    return true;
}

} // namespace hush
