#pragma once

#include <cstddef>
#include <vector>

namespace hush {

enum class AutomationInterpolation { Step, Linear };

struct AutomationPoint {
    double beat{};
    float value{};
};

// Build a lane on the control thread, then treat it as immutable while audio is
// running. evaluate() is allocation-free and safe for the audio thread only when
// no other thread mutates the lane. Values outside the point range hold endpoints.
class AutomationLane {
public:
    [[nodiscard]] bool set_points(std::vector<AutomationPoint> points,
                                  AutomationInterpolation interpolation) noexcept;
    [[nodiscard]] float evaluate(double beat) const noexcept;
    [[nodiscard]] std::size_t point_count() const noexcept { return points_.size(); }
    [[nodiscard]] AutomationInterpolation interpolation() const noexcept { return interpolation_; }

private:
    std::vector<AutomationPoint> points_;
    AutomationInterpolation interpolation_{AutomationInterpolation::Linear};
};

} // namespace hush
