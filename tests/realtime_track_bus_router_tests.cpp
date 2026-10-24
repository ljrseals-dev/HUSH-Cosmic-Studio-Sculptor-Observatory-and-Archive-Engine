#include "hush/bus_graph.hpp"
#include "hush/realtime_track_bus_router.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>

namespace {
void near(float actual, float expected) {
    assert(std::abs(actual - expected) < 1.0e-5F);
}
}

int main() {
    using namespace hush;
    RealtimeTrackBusRouter router;
    assert(!router.configure({0, 2}, 2, 4));
    assert(router.configure({0, 1, 0}, 2, 4));
    assert(!router.configure({0}, 1, 4));

    const float a[] = {1.0F, 0.5F};
    const float b[] = {1.0F, 1.0F};
    const float c[] = {0.25F, 0.25F};
    const float* tracks[] = {a, b, c};
    assert(router.process(tracks, 3, 2));
    const auto* buses = router.bus_inputs();
    assert(router.bus_count() == 2);
    // Tracks 0 and 2 sum to bus 0; track 1 is isolated on bus 1.
    near(buses[0].left[0], 1.25F * 0.70710678F);
    near(buses[0].right[0], 1.25F * 0.70710678F);
    near(buses[1].left[0], 0.70710678F);
    near(buses[1].right[0], 0.70710678F);

    // Mute applies at the next block and only removes the selected track.
    assert(router.enqueue_control(2, 1.0F, 0.0F, true, false));
    assert(router.process(tracks, 3, 2));
    buses = router.bus_inputs();
    near(buses[0].left[0], 0.70710678F);
    near(buses[0].right[0], 0.70710678F);

    // Solo is global across tracks, independent of their destination bus.
    assert(router.enqueue_control(1, 1.0F, -1.0F, false, true));
    assert(router.process(tracks, 3, 2));
    buses = router.bus_inputs();
    near(buses[0].left[0], 0.0F);
    near(buses[0].right[0], 0.0F);
    near(buses[1].left[0], 1.0F);
    near(buses[1].right[0], 0.0F);

    // Invalid blocks fail without dereferencing caller buffers.
    assert(!router.process(tracks, 2, 2));
    assert(!router.process(tracks, 3, 5));
    return 0;
}
