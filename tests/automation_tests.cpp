#include "hush/automation.hpp"

#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

int main() {
    using namespace hush;
    AutomationLane lane;
    assert(lane.snapshot().empty());
    assert(lane.snapshot().evaluate(100) == 0.0F);

    assert(lane.replace_points({
        {0, 0.0F, AutomationCurve::Linear},
        {100, 1.0F, AutomationCurve::Step},
        {200, 0.25F, AutomationCurve::Linear}
    }));
    auto playback = lane.snapshot();
    assert(playback.size() == 3);
    assert(std::abs(playback.evaluate(0) - 0.0F) < 1.0e-6F);
    assert(std::abs(playback.evaluate(50) - 0.5F) < 1.0e-6F);
    assert(std::abs(playback.evaluate(100) - 1.0F) < 1.0e-6F);
    assert(std::abs(playback.evaluate(150) - 1.0F) < 1.0e-6F);
    assert(std::abs(playback.evaluate(200) - 0.25F) < 1.0e-6F);
    assert(std::abs(playback.evaluate(999) - 0.25F) < 1.0e-6F);

    // Editing is non-realtime; an existing snapshot stays unchanged.
    assert(lane.upsert({50, 0.75F, AutomationCurve::Linear}));
    assert(std::abs(playback.evaluate(50) - 0.5F) < 1.0e-6F);
    const auto edited = lane.snapshot();
    assert(std::abs(edited.evaluate(50) - 0.75F) < 1.0e-6F);

    // Replacing with duplicate positions, non-finite values, or invalid curves
    // is rejected without corrupting the previous lane.
    assert(!lane.replace_points({{5, 0.0F, AutomationCurve::Linear},
                                 {5, 1.0F, AutomationCurve::Linear}}));
    assert(!lane.upsert({75, std::numeric_limits<float>::quiet_NaN(),
                         AutomationCurve::Linear}));
    assert(!lane.upsert({75, 0.5F, static_cast<AutomationCurve>(99)}));
    assert(lane.points().size() == 4);
    return 0;
}
