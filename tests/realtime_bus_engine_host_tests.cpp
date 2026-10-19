#include "hush/realtime_bus_engine_host.hpp"

#include <atomic>
#include <cassert>
#include <cmath>
#include <thread>

namespace {
hush::BusGraph make_graph(float route_gain) {
    hush::BusGraph graph;
    hush::RoutingError error;
    const bool ok = graph.configure(
        {{"input", "Input"}, {"master", "Master"}},
        {{"input", "master", hush::SendTap::PostFader, route_gain}}, error);
    assert(ok);
    return graph;
}
}

int main() {
    using namespace hush;
    RealtimeBusEngineHost host;
    ProcessorSetupError error;
    auto graph = make_graph(0.5F);
    assert(host.replace_graph(graph, 16, "master", error));
    assert(host.bus_count_for_control() == 2U);

    const float input_left[4]{1.0F, 0.5F, -1.0F, 0.25F};
    const float input_right[4]{0.25F, -0.5F, 1.0F, 0.0F};
    const float silence[4]{};
    const BusAudioInputView inputs[2]{{input_left, input_right}, {silence, silence}};
    float output_left[4]{};
    float output_right[4]{};

    assert(host.process_to_stereo(inputs, 2, 4, output_left, output_right) == RealtimeProcessStatus::Ok);
    assert(std::abs(output_left[0] - 0.5F) < 1.0e-6F);
    assert(std::abs(output_right[0] - 0.125F) < 1.0e-6F);

    // Invalid replacement must not displace the active, working graph.
    assert(!host.replace_graph(graph, 16, "missing-master", error));
    assert(host.process_to_stereo(inputs, 2, 4, output_left, output_right) == RealtimeProcessStatus::Ok);
    assert(std::abs(output_left[0] - 0.5F) < 1.0e-6F);

    // The control thread repeatedly publishes fully constructed snapshots while the
    // single audio reader processes. Every output must come from one valid snapshot.
    std::atomic<bool> stop{false};
    std::atomic<unsigned int> blocks{0U};
    std::atomic<bool> invalid_output{false};
    std::thread audio([&] {
        while (!stop.load(std::memory_order_acquire)) {
            const auto status = host.process_to_stereo(inputs, 2, 4, output_left, output_right);
            if (status != RealtimeProcessStatus::Ok) {
                invalid_output.store(true, std::memory_order_relaxed);
                break;
            }
            const float gain = output_left[0];
            // The initial valid snapshot (0.5) may be observed before the
            // control thread publishes the first replacement.
            if (std::abs(gain - 0.25F) > 1.0e-6F &&
                std::abs(gain - 0.5F) > 1.0e-6F &&
                std::abs(gain - 0.75F) > 1.0e-6F) {
                invalid_output.store(true, std::memory_order_relaxed);
                break;
            }
            blocks.fetch_add(1U, std::memory_order_relaxed);
        }
    });

    for (unsigned int i = 0; i < 1000U; ++i) {
        auto replacement = make_graph((i % 2U) == 0U ? 0.25F : 0.75F);
        assert(host.replace_graph(replacement, 16, "master", error));
        if ((i % 13U) == 0U) host.reclaim_retired();
    }
    stop.store(true, std::memory_order_release);
    audio.join();
    host.reclaim_retired();
    assert(!invalid_output.load(std::memory_order_relaxed));
    assert(blocks.load(std::memory_order_relaxed) > 0U);
    return 0;
}
