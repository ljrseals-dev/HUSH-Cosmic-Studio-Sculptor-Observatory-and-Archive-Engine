#include "hush/realtime_track_bus_router.hpp"

#include <cassert>
#include <cmath>

namespace {
bool near(float a, float b) { return std::abs(a - b) < 1.0e-5F; }
}

int main() {
    using namespace hush;
    RealtimeTrackBusRouter router;
    assert(!router.configure({0U, 2U}, 2U, 4U));
    assert(router.configure({0U, 1U, 0U}, 2U, 4U));
    assert(!router.configure({0U}, 1U, 4U));

    const float kick[] = {1.0F, 0.5F};
    const float bass[] = {0.5F, 0.25F};
    const float vocal[] = {1.0F, 1.0F};
    const float* tracks[] = {kick, bass, vocal};
    assert(router.process(tracks, 3U, 2U));
    auto* buses = router.bus_inputs();
    assert(router.bus_count() == 2U);
    assert(near(buses[0].left[0], std::sqrt(0.5F) * 2.0F));
    assert(near(buses[0].right[0], std::sqrt(0.5F) * 2.0F));
    assert(near(buses[1].left[0], std::sqrt(0.125F)));
    assert(near(buses[1].right[0], std::sqrt(0.125F)));

    // Updates are consumed only when the next block is processed.
    assert(router.enqueue_track_control(0U, 0.5F, -1.0F, false, false));
    assert(router.enqueue_track_control(1U, 1.0F, 0.0F, true, false));
    assert(router.enqueue_track_control(2U, 1.0F, 0.0F, true, false));
    assert(router.process(tracks, 3U, 2U));
    buses = router.bus_inputs();
    assert(near(buses[0].left[0], 0.5F));
    assert(near(buses[0].right[0], 0.0F));
    assert(near(buses[1].left[0], 0.0F));
    assert(near(buses[1].right[0], 0.0F));

    // Mute wins over solo; solo otherwise excludes non-solo tracks.
    assert(router.enqueue_track_control(0U, 1.0F, 0.0F, false, false));
    assert(router.enqueue_track_control(1U, 1.0F, 0.0F, false, false));
    assert(router.enqueue_track_control(2U, 1.0F, 0.0F, true, true));
    assert(router.process(tracks, 3U, 2U));
    buses = router.bus_inputs();
    for (std::size_t bus = 0; bus < router.bus_count(); ++bus) {
        assert(near(buses[bus].left[0], 0.0F));
        assert(near(buses[bus].right[0], 0.0F));
    }

    assert(router.enqueue_track_control(2U, 1.0F, 0.0F, false, true));
    assert(router.process(tracks, 3U, 2U));
    buses = router.bus_inputs();
    assert(near(buses[0].left[0], std::sqrt(0.5F)));
    assert(near(buses[0].right[0], std::sqrt(0.5F)));
    assert(!router.enqueue_track_control(3U, 1.0F, 0.0F, false, false));
    assert(!router.enqueue_track_control(0U, 1.0F, 2.0F, false, false));
    assert(!router.process(tracks, 2U, 2U));
    assert(!router.process(tracks, 3U, 5U));

    RealtimeTrackBusRouter saturated;
    assert(saturated.configure({0U}, 1U, 2U));
    for (std::size_t i = 0; i < RealtimeTrackBusRouter::kQueueCapacity; ++i)
        assert(saturated.enqueue_track_control(0U, 1.0F, 0.0F, false, false));
    assert(!saturated.enqueue_track_control(0U, 0.5F, 0.0F, false, false));
    const float* one[] = {kick};
    assert(saturated.process(one, 1U, 2U));
    assert(saturated.enqueue_track_control(0U, 0.5F, 0.0F, false, false));
    return 0;
}
