#include "hush/realtime_graph_publisher.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>
#include <thread>

struct ImmutableGraph {
    explicit ImmutableGraph(unsigned int value_in) : value(value_in) {}
    const unsigned int value;
};

int main() {
    hush::RealtimeGraphPublisher<ImmutableGraph> publisher;
    publisher.publish(std::make_unique<ImmutableGraph>(1U));
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
        publisher.publish(std::make_unique<ImmutableGraph>(value));
        if ((value % 7U) == 0U) std::this_thread::yield();
    }
    stop.store(true, std::memory_order_release);
    audio.join();
    publisher.reclaim_retired();
    assert(observed.load(std::memory_order_relaxed) >= 1U);
    assert(publisher.retired_count_for_control() == 0U);
    return 0;
}
