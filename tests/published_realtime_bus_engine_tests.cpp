#include "hush/published_realtime_bus_engine.hpp"

#include <atomic>
#include <cassert>
#include <thread>
#include <vector>

namespace {

hush::BusGraph make_graph() {
    hush::BusGraph graph;
    hush::RoutingError error;
    const bool configured = graph.configure({{"master", "Master"}}, {}, error);
    assert(configured);
    return graph;
}

void publish_and_process_contract() {
    hush::PublishedRealtimeBusEngine engine;
    hush::ProcessorSetupError error;
    auto graph = make_graph();
    assert(engine.publish_graph(graph, 16, error));

    float left[4]{1.0F, 0.5F, -0.25F, 0.0F};
    float right[4]{0.25F, 0.5F, 1.0F, -1.0F};
    const hush::BusAudioInputView inputs[]{{left, right}};
    float output_left[4]{};
    float output_right[4]{};
    assert(engine.process_to_master(inputs, 1, 4, 0, output_left, output_right) ==
           hush::RealtimeProcessStatus::Ok);
    assert(output_left[0] == left[0] && output_left[1] == left[1]);
    assert(output_right[0] == right[0] && output_right[1] == right[1]);

    assert(engine.process(inputs, 1, 4) == hush::RealtimeProcessStatus::Ok);
    assert(engine.enqueue_fader_change(0, 0.5F));
    // The queued change is consumed by the next process block.
    assert(engine.process(inputs, 1, 4) == hush::RealtimeProcessStatus::Ok);
    assert(!engine.enqueue_fader_change(1, 1.0F));
    assert(engine.process(inputs, 1, 17) ==
           hush::RealtimeProcessStatus::FrameCountExceedsCapacity);
}

void concurrent_engine_replacement_contract() {
    hush::PublishedRealtimeBusEngine engine;
    hush::ProcessorSetupError error;
    auto graph = make_graph();
    assert(engine.publish_graph(graph, 16, error));

    float left[8]{0.125F, 0.25F, 0.375F, 0.5F, 0.625F, 0.75F, 0.875F, 1.0F};
    float right[8]{-0.125F, -0.25F, -0.375F, -0.5F, -0.625F, -0.75F, -0.875F, -1.0F};
    const hush::BusAudioInputView inputs[]{{left, right}};
    std::atomic<bool> stop{false};
    std::atomic<unsigned int> successful_blocks{0U};
    std::atomic<unsigned int> copy_failures{0U};

    // Exercise the real device-output path during publication, not just process().
    // The returned master view must be copied before its graph lifetime guard ends.
    std::thread audio([&] {
        float output_left[8]{};
        float output_right[8]{};
        while (!stop.load(std::memory_order_acquire)) {
            const auto status = engine.process_to_master(
                inputs, 1, 8, 0, output_left, output_right);
            if (status != hush::RealtimeProcessStatus::Ok) {
                copy_failures.fetch_add(1U, std::memory_order_relaxed);
                continue;
            }
            for (std::size_t i = 0; i < 8U; ++i) {
                if (output_left[i] != left[i] || output_right[i] != right[i]) {
                    copy_failures.fetch_add(1U, std::memory_order_relaxed);
                    break;
                }
            }
            successful_blocks.fetch_add(1U, std::memory_order_relaxed);
        }
    });

    for (unsigned int i = 0; i < 2000U; ++i) {
        assert(engine.publish_graph(graph, 16, error));
        if ((i % 11U) == 0U) engine.reclaim_retired();
    }
    stop.store(true, std::memory_order_release);
    audio.join();
    engine.reclaim_retired();

    assert(successful_blocks.load(std::memory_order_relaxed) > 0U);
    assert(copy_failures.load(std::memory_order_relaxed) == 0U);
    assert(engine.retired_count_for_control() == 0U);
}

} // namespace

int main() {
    publish_and_process_contract();
    concurrent_engine_replacement_contract();
    return 0;
}
