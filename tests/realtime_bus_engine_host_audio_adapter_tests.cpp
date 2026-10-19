#include "hush/realtime_bus_engine_host_audio_adapter.hpp"

#include <atomic>
#include <cassert>
#include <cmath>
#include <string>
#include <thread>

namespace {
hush::BusGraph make_graph(float gain) {
    hush::BusGraph graph;
    hush::RoutingError error;
    assert(graph.configure({{"input", "Input"}, {"master", "Master"}},
                           {{"input", "master", hush::SendTap::PostFader, gain}},
                           error));
    return graph;
}
}

int main() {
    using namespace hush;
    RealtimeBusEngineHost host;
    ProcessorSetupError setup_error;
    auto initial = make_graph(1.0F);
    assert(host.replace_graph(initial, 32, "master", setup_error));

    RealtimeBusEngineHostAudioAdapter adapter;
    std::string error;
    assert(adapter.configure(host, 0, 2, 32, error));
    assert(error.empty());

    const float input[] = {1.0F, 0.5F, -1.0F, -0.25F, 0.5F, 1.0F};
    float output[6]{};
    assert(adapter.process(input, output, 3));
    for (std::size_t i = 0; i < 6; ++i)
        assert(std::abs(output[i] - input[i]) < 1.0e-6F);

    // A same-topology graph replacement is safe while the adapter remains active.
    auto quieter = make_graph(0.25F);
    assert(host.replace_graph(quieter, 32, "master", setup_error));
    assert(adapter.process(input, output, 3));
    assert(std::abs(output[0] - 0.25F) < 1.0e-6F);
    assert(std::abs(output[1] - 0.125F) < 1.0e-6F);

    std::atomic<bool> stop{false};
    std::atomic<bool> failed{false};
    std::thread audio([&] {
        float local_output[6]{};
        while (!stop.load(std::memory_order_acquire)) {
            if (!adapter.process(input, local_output, 3)) {
                failed.store(true, std::memory_order_relaxed);
                break;
            }
            const float first = local_output[0];
            if (std::abs(first - 0.25F) > 1.0e-6F &&
                std::abs(first - 0.75F) > 1.0e-6F) {
                failed.store(true, std::memory_order_relaxed);
                break;
            }
        }
    });
    for (unsigned int i = 0; i < 500U; ++i) {
        auto replacement = make_graph((i % 2U) == 0U ? 0.75F : 0.25F);
        assert(host.replace_graph(replacement, 32, "master", setup_error));
        if ((i % 7U) == 0U) host.reclaim_retired();
    }
    stop.store(true, std::memory_order_release);
    audio.join();
    host.reclaim_retired();
    assert(!failed.load(std::memory_order_relaxed));

    float bad_output[] = {9.0F, 9.0F};
    assert(!adapter.process(nullptr, bad_output, 1));
    assert(bad_output[0] == 0.0F && bad_output[1] == 0.0F);
    return 0;
}
