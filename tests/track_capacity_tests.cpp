#include "hush/project.hpp"
#include "hush/realtime_track_mixer.hpp"
#include "hush/track_limits.hpp"

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <vector>

namespace {
std::atomic<bool> allocation_guard{false};
std::atomic<std::size_t> guarded_allocations{0};
}

void* operator new(std::size_t size) {
    if (allocation_guard.load(std::memory_order_relaxed))
        guarded_allocations.fetch_add(1U, std::memory_order_relaxed);
    if (void* p = std::malloc(size == 0U ? 1U : size)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) {
    return ::operator new(size);
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {
hush::Project make_project(std::size_t count) {
    hush::Project project;
    project.id = "track-capacity-contract";
    project.tracks.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto id = "track-" + std::to_string(i);
        project.tracks.push_back({id, id, 1.0F, 0.0F, false, false});
    }
    return project;
}
}

int main() {
    using namespace hush;
    static_assert(kMaxTracks == static_cast<std::size_t>(HUSH_MAX_TRACKS));
    static_assert(kMaxTracks > 0U && kMaxTracks <= 65535U);

    for (const std::size_t capacity : {1U, 8U, 64U, 1024U}) {
        if (capacity <= kMaxTracks) {
            RealtimeTrackMixer mixer;
            assert(mixer.configure(capacity, 8U));
            assert(mixer.track_count() == capacity);
        }
    }

    RealtimeTrackMixer over_capacity;
    assert(!over_capacity.configure(kMaxTracks + 1U, 8U));

    // Schema-v1 round trip succeeds at the ceiling and rejects the next track.
    Project at_limit = make_project(kMaxTracks);
    Project decoded;
    Error error;
    assert(from_json(to_json(at_limit), decoded, error));
    assert(decoded.tracks.size() == kMaxTracks);
    Project over_limit = make_project(kMaxTracks + 1U);
    assert(!from_json(to_json(over_limit), decoded, error));
    assert(error.code == "track_limit_exceeded");
    assert(error.message.find("HUSH_MAX_TRACKS") != std::string::npos);

    // Full-capacity audio processing must not call global operator new.
    RealtimeTrackMixer mixer;
    constexpr std::size_t frames = 8U;
    assert(mixer.configure(kMaxTracks, frames));
    std::vector<std::vector<float>> owned(kMaxTracks, std::vector<float>(frames, 0.001F));
    std::vector<const float*> inputs(kMaxTracks);
    for (std::size_t i = 0; i < kMaxTracks; ++i) inputs[i] = owned[i].data();
    std::vector<float> left(frames), right(frames);
    assert(mixer.enqueue_control(kMaxTracks - 1U, 0.75F, 0.25F, false, false));

    guarded_allocations.store(0U, std::memory_order_relaxed);
    allocation_guard.store(true, std::memory_order_relaxed);
    const bool processed = mixer.process(inputs.data(), inputs.size(), left.data(), right.data(), frames);
    allocation_guard.store(false, std::memory_order_relaxed);
    assert(processed);
    assert(guarded_allocations.load(std::memory_order_relaxed) == 0U);
    return 0;
}
