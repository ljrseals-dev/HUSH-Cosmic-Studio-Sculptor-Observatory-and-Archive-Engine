#include "hush/realtime_track_mixer.hpp"

#include <cassert>
#include <cmath>
#include <limits>

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
    assert(std::abs(mixer.output_peak() - 1.4142135F) < 1.0e-5F);
    assert(std::abs(mixer.output_rms() - 1.4142135F) < 1.0e-5F);

    // Meter values describe the most recently processed block.
    assert(mixer.process(inputs, 2, left, right, 0));
    assert(mixer.output_peak() == 0.0F);
    assert(mixer.output_rms() == 0.0F);
    assert(mixer.process(inputs, 2, left, right, 2));

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
    assert(!mixer.enqueue_control(0, 2.01F, 0.0F, false, false));
    assert(!mixer.enqueue_control(0, 1.0F, 2.0F, false, false));
    assert(!mixer.enqueue_control(0, 1.0F, -2.0F, false, false));
    assert(!mixer.enqueue_control(0, std::numeric_limits<float>::quiet_NaN(), 0.0F, false, false));
    assert(!mixer.enqueue_control(0, 1.0F, std::numeric_limits<float>::infinity(), false, false));
    assert(!mixer.enqueue_control(99, 1.0F, 0.0F, false, false));
    assert(!mixer.process(inputs, 1, left, right, 2));
    assert(!mixer.process(inputs, 2, left, right, 9));

    // The SPSC command queue is bounded: fill it without a consumer, then
    // verify the next command is rejected rather than blocking or overwriting.
    RealtimeTrackMixer saturated;
    assert(saturated.configure(1, 2));
    for (std::size_t i = 0; i < RealtimeTrackMixer::kQueueCapacity; ++i) {
        assert(saturated.enqueue_control(0, 1.0F, 0.0F, false, false));
    }
    assert(!saturated.enqueue_control(0, 0.5F, 0.0F, false, false));
    const float* one_input[] = {a};
    assert(saturated.process(one_input, 1, left, right, 2));
    assert(saturated.enqueue_control(0, 0.5F, 0.0F, false, false));
    return 0;
}
