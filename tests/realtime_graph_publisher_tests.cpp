#include "hush/realtime_graph_publisher.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>
#include <thread>

struct ImmutableGraph {
    ImmutableGraph(unsigned int value_in, std::shared_ptr<std::atomic<unsigned int>> destroyed_in)
        : value(value_in), destroyed(std::move(destroyed_in)) {}
    ~ImmutableGraph() { destroyed->fetch_add(1U, std::memory_order_relaxed); }

    const unsigned int value;
    std::shared_ptr<std::atomic<unsigned int>> destroyed;
};

static void held_reader_defers_reclamation() {
    auto destroyed = std::make_shared<std::atomic<unsigned int>>(0U);
    hush::RealtimeGraphPublisher<ImmutableGraph> publisher;
    publisher.publish(std::make_unique<ImmutableGraph>(1U, destroyed));

    {
        auto guard = publisher.acquire_for_audio();
        assert(guard && guard->value == 1U);
        // The primitive is single-reader; a second/nested guard must fail closed
        // without clearing the first reader's reclamation hazard.
        auto overlapping = publisher.acquire_for_audio();
        assert(!overlapping);

        publisher.publish(std::make_unique<ImmutableGraph>(2U, destroyed));
        // Publishing must not destroy the old snapshot while an audio reader
        // could still hold its pointer.
        assert(destroyed->load(std::memory_order_relaxed) == 0U);
        assert(publisher.retired_count_for_control() == 1U);
        assert(guard->value == 1U);
    }

    publisher.reclaim_retired();
    assert(destroyed->load(std::memory_order_relaxed) == 1U);
    assert(publisher.retired_count_for_control() == 0U);
}

static void concurrent_publication_stress() {
    auto destroyed = std::make_shared<std::atomic<unsigned int>>(0U);
    hush::RealtimeGraphPublisher<ImmutableGraph> publisher;
    publisher.publish(std::make_unique<ImmutableGraph>(1U, destroyed));
    std::atomic<bool> stop{false};
    std::atomic<unsigned int> observed{0U};

    std::thread audio([&] {
        while (!stop.load(std::memory_order_acquire)) {
            auto guard = publisher.acquire_for_audio();
            if (guard) {
                const auto value = guard->value;
                assert(value >= 1U);
                observed.store(value, std::memory_order_relaxed);
            }
        }
    });

    for (unsigned int value = 2U; value < 20000U; ++value) {
        publisher.publish(std::make_unique<ImmutableGraph>(value, destroyed));
        if ((value % 7U) == 0U) std::this_thread::yield();
    }
    stop.store(true, std::memory_order_release);
    audio.join();
    publisher.reclaim_retired();
    assert(observed.load(std::memory_order_relaxed) >= 1U);
    assert(publisher.retired_count_for_control() == 0U);
}

int main() {
    held_reader_defers_reclamation();
    concurrent_publication_stress();
    return 0;
}
