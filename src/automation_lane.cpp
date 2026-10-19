#include "hush/automation_lane.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace hush {

bool AutomationLane::set_points(std::vector<AutomationPoint> points,
                                AutomationInterpolation interpolation) noexcept {
    if (points.empty()) return false;
    double previous = -1.0;
    for (const auto& point : points) {
        if (!std::isfinite(point.beat) || !std::isfinite(point.value) ||
            point.beat < 0.0 || point.beat <= previous) return false;
        previous = point.beat;
    }
    points_ = std::move(points);
    interpolation_ = interpolation;
    return true;
}

float AutomationLane::evaluate(double beat) const noexcept {
    if (points_.empty() || !std::isfinite(beat)) return 0.0F;
    if (beat <= points_.front().beat) return points_.front().value;
    if (beat >= points_.back().beat) return points_.back().value;

    const auto right = std::upper_bound(
        points_.begin(), points_.end(), beat,
        [](double value, const AutomationPoint& point) { return value < point.beat; });
    const auto left = right - 1;
    if (interpolation_ == AutomationInterpolation::Step) return left->value;

    const double span = right->beat - left->beat;
    const double proportion = (beat - left->beat) / span;
    return static_cast<float>(static_cast<double>(left->value) +
        (static_cast<double>(right->value) - left->value) * proportion);
}

} // namespace hush
