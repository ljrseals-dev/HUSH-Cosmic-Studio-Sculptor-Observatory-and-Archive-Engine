#include "hush/realtime_bus_audio_adapter.hpp"

#include <cassert>
#include <cmath>
#include <string>

int main() {
    using namespace hush;
    RoutingError routing_error;
    BusGraph graph;
    assert(graph.configure({{"input", "Input"}, {"master", "Master"}},
                           {{"input", "master", SendTap::PostFader, 1.0F}},
                           routing_error));

    RealtimeBusEngine engine;
    ProcessorSetupError setup_error;
    assert(engine.configure(graph, 16, setup_error));

    RealtimeBusAudioAdapter adapter;
    std::string error;
    assert(adapter.configure(engine, 0, 1, 16, error));
    assert(error.empty());

    const float input[] = {1.0F, 0.5F, 2.0F, 1.0F, -1.0F, -0.5F};
    float output[] = {99.0F, 99.0F, 99.0F, 99.0F, 99.0F, 99.0F};
    assert(adapter.process(input, output, 3));
    for (std::size_t i = 0; i < 6; ++i)
        assert(std::abs(output[i] - input[i]) < 1.0e-6F);

    assert(engine.enqueue_fader_change(0, 0.5F));
    assert(adapter.process(input, output, 3));
    assert(std::abs(output[0] - 0.5F) < 1.0e-6F);
    assert(std::abs(output[1] - 0.25F) < 1.0e-6F);

    float bad_output[] = {7.0F, 7.0F};
    assert(!adapter.process(nullptr, bad_output, 1));
    assert(bad_output[0] == 0.0F && bad_output[1] == 0.0F);

    RealtimeBusAudioAdapter bad_adapter;
    assert(!bad_adapter.configure(engine, 8, 1, 16, error));
    assert(error == "adapter_bus_index_out_of_range");

    RealtimeBusAudioAdapter::alsa_callback(input, output, 3, &adapter);
    return 0;
}
