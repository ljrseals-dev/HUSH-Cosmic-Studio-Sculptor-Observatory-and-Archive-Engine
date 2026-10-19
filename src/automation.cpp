#include "hush/automation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace hush {

AutomationSnapshot::AutomationSnapshot(std::vector<AutomationPoint> points)
    : points_(std::move(points)) {}

float AutomationSnapshot::evaluate(std::uint64_t position_samples) const noexcept {
    if (points_.empty()) return 0.0F;
    if (position_samples <= points_.front().position_samples) return points_.front().value;
    if (position_samples >= points_.back().position_samples) return points_.back().value;

    const auto right = std::upper_bound(
        points_.begin(), points_.end(), position_samples,
        [](std::uint64_t position, const AutomationPoint& point) {
            return position < point.position_samples;
        });
    const auto left = right - 1;
    if (left->curve_to_next == AutomationCurve::Step) return left->value;

    const auto span = right->position_samples - left->position_samples;
    const auto offset = position_samples - left->position_samples;
    const double fraction = static_cast<double>(offset) / static_cast<double>(span);
    const double from = static_cast<double>(left->value);
    const double to = static_cast<double>(right->value);
    return static_cast<float>(from + (to - from) * fraction);
}

bool AutomationLane::valid_point(const AutomationPoint& point) noexcept {
    return std::isfinite(point.value) &&
           (point.curve_to_next == AutomationCurve::Step ||
            point.curve_to_next == AutomationCurve::Linear);
}

bool AutomationLane::valid_sequence(const std::vector<AutomationPoint>& points) noexcept {
    for (std::size_t i = 0; i < points.size(); ++i) {
        if (!valid_point(points[i])) return false;
        if (i != 0 && points[i - 1].position_samples >= points[i].position_samples)
            return false;
    }
    return true;
}

bool AutomationLane::replace_points(std::vector<AutomationPoint> points) {
    if (!valid_sequence(points)) return false;
    points_.swap(points);
    return true;
}

bool AutomationLane::upsert(AutomationPoint point) {
    if (!valid_point(point)) return false;
    const auto it = std::lower_bound(
        points_.begin(), points_.end(), point.position_samples,
        [](const AutomationPoint& existing, std::uint64_t position) {
            return existing.position_samples < position;
        });
    if (it != points_.end() && it->position_samples == point.position_samples) {
        *it = point;
        return true;
    }
    points_.insert(it, point);
    return true;
}

AutomationSnapshot AutomationLane::snapshot() const {
    return AutomationSnapshot(points_);
}

} // namespace hush
