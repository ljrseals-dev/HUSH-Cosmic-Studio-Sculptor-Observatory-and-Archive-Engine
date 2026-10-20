#include "hush/realtime_track_mixer.hpp"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <new>
#include <vector>

namespace {
bool allocation_guard_enabled = false;
std::size_t guarded_allocations = 0;
}

void* operator new(std::size_t size) {
    if (allocation_guard_enabled) ++guarded_allocations;
    if (void* memory = std::malloc(size == 0U ? 1U : size)) return memory;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) {
    return ::operator new(size);
}
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

int main() {
    using namespace hush;
    RealtimeTrackMixer mixer;
    assert(mixer.configure(2, 8));
    assert(!mixer.configure(2, 8));
    RealtimeTrackMixer over_capacity;
    assert(!over_capacity.configure(kMaxTracks + 1U, 8));

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

    // Configure full capacity and all caller-owned buffers before enabling the
    // allocation guard. The realtime process call must not invoke operator new.
    RealtimeTrackMixer maximum;
    constexpr std::size_t frames = 128;
    assert(maximum.configure(kMaxTracks, frames));
    std::vector<std::vector<float>> full_buffers(
        kMaxTracks, std::vector<float>(frames, 0.125F));
    std::vector<const float*> full_inputs(kMaxTracks);
    for (std::size_t i = 0; i < kMaxTracks; ++i)
        full_inputs[i] = full_buffers[i].data();
    std::vector<float> full_left(frames, 0.0F), full_right(frames, 0.0F);
    const auto allocations_before = guarded_allocations;
    allocation_guard_enabled = true;
    const bool processed = maximum.process(full_inputs.data(), kMaxTracks,
                                           full_left.data(), full_right.data(), frames);
    allocation_guard_enabled = false;
    assert(processed);
    assert(guarded_allocations == allocations_before);
    return 0;
}
