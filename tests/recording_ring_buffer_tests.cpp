#include "hush/recording_ring_buffer.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <span>

int main() {
    using hush::RecordingRingBuffer;
    RecordingRingBuffer ring;
    assert(!ring.configure(0U, 2U));
    assert(!ring.configure(8U, 0U));
    assert(!ring.configure(8U, 9U));
    assert(ring.configure(3U, 2U));
    assert(ring.capacity_frames() == 3U && ring.channels() == 2U);
    assert(ring.frames_free() == 3U && ring.frames_available() == 0U);

    const std::array<float, 4> first{1.0F, 10.0F, 2.0F, 20.0F};
    assert(ring.capture(first) == 2U);
    assert(ring.frames_available() == 2U && ring.frames_free() == 1U);

    std::array<float, 2> one_frame{};
    assert(ring.drain(one_frame) == 1U);
    assert(one_frame[0] == 1.0F && one_frame[1] == 10.0F);

    const std::array<float, 6> second{3.0F, 30.0F, 4.0F, 40.0F, 5.0F, 50.0F};
    assert(ring.capture(second) == 2U);
    assert(ring.dropped_frames() == 1U);
    assert(ring.frames_available() == 3U);

    std::array<float, 8> drained{};
    assert(ring.drain(drained) == 3U);
    const std::array<float, 6> expected{2.0F, 20.0F, 3.0F, 30.0F, 4.0F, 40.0F};
    for (std::size_t i = 0; i < expected.size(); ++i) assert(drained[i] == expected[i]);
    assert(ring.frames_available() == 0U && ring.frames_free() == 3U);

    assert(ring.capture(std::span<const float>(first.data(), 3U)) == 0U);
    assert(ring.drain(std::span<float>(drained.data(), 1U)) == 0U);
    assert(ring.dropped_frames() == 1U);

    assert(ring.configure(2U, 1U));
    const std::array<float, 2> mono{0.25F, -0.25F};
    assert(ring.capture(mono) == 2U);
    std::array<float, 2> mono_out{};
    assert(ring.drain(mono_out) == 2U);
    assert(mono_out == mono);
    return 0;
}
