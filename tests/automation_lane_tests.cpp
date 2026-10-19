#include "hush/automation_lane.hpp"

#include <cassert>
#include <cmath>
#include <limits>

int main() {
    using namespace hush;
    AutomationLane lane{
        "track-1", "gain",
        {{100, 1.0F, AutomationCurve::Linear},
         {0, 0.0F, AutomationCurve::Linear},
         {200, 0.5F, AutomationCurve::Hold}}
    };
    assert(normalize_automation_lane(lane));
    assert(lane.points.front().position_samples == 0);
    AutomationLaneView view(lane);
    assert(std::abs(view.evaluate(0) - 0.0F) < 1.0e-6F);
    assert(std::abs(view.evaluate(50) - 0.5F) < 1.0e-6F);
    assert(std::abs(view.evaluate(100) - 1.0F) < 1.0e-6F);
    assert(std::abs(view.evaluate(150) - 0.75F) < 1.0e-6F);
    assert(std::abs(view.evaluate(200) - 0.5F) < 1.0e-6F);
    assert(std::abs(view.evaluate(300) - 0.5F) < 1.0e-6F);

    AutomationLane empty{"track-1", "pan", {}};
    assert(normalize_automation_lane(empty));
    assert(AutomationLaneView(empty).evaluate(12, 0.25F) == 0.25F);

    AutomationLane duplicate{
        "track-1", "gain",
        {{8, 0.1F, AutomationCurve::Linear},
         {8, 0.2F, AutomationCurve::Linear}}
    };
    assert(!normalize_automation_lane(duplicate));

    AutomationLane invalid{"track-1", "gain",
                            {{0, std::numeric_limits<float>::infinity(),
                              AutomationCurve::Linear}}};
    assert(!normalize_automation_lane(invalid));
    return 0;
}
