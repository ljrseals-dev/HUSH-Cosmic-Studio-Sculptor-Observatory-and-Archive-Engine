#include "hush/realtime_track_mixer.hpp"

#include <cassert>
#include <cmath>

int main() {
    using namespace hush;
    RealtimeTrackMixer mixer;
    assert(mixer.configure(2, 8));
    assert(!mixer.configure(2, 8));

    const float a[] = {1.0F, 0.5F};
    const float b[] = {1.0F, 1.0F};
    const float* inputs[] = {a, b};
    float left[2]{}, right[2]{};

    assert(mixer.process(inputs, 2, left, right, 2));
    // Equal-power center pan, unity gain, two tracks summed.
    assert(std::abs(left[0] - 1.4142135F) < 1.0e-5F);
    assert(std::abs(right[0] - 1.4142135F) < 1.0e-5F);

    // Commands are applied at the next process block boundary.
    assert(mixer.enqueue_control(0, 0.5F, -1.0F, false, false));
    assert(mixer.enqueue_control(1, 1.0F, 0.0F, true, false));
    assert(mixer.process(inputs, 2, left, right, 2));
    assert(std::abs(left[0] - 0.5F) < 1.0e-5F);
    assert(std::abs(right[0]) < 1.0e-5F);

    assert(mixer.enqueue_control(0, 1.0F, 0.0F, false, false));
    assert(mixer.enqueue_control(1, 1.0F, 0.0F, false, true));
    assert(mixer.process(inputs, 2, left, right, 2));
    assert(std::abs(left[0] - 0.7071067F) < 1.0e-5F);
    assert(std::abs(right[0] - 0.7071067F) < 1.0e-5F);

    assert(!mixer.enqueue_control(0, -1.0F, 0.0F, false, false));
    assert(!mixer.enqueue_control(0, 1.0F, 2.0F, false, false));
    assert(!mixer.process(inputs, 1, left, right, 2));
    assert(!mixer.process(inputs, 2, left, right, 9));
    return 0;
}
