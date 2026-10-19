#include "hush/bus_renderer.hpp"
#include <cassert>
#include <cmath>

int main() {
    using namespace hush;
    RoutingError graph_error;
    BusGraph graph;
    assert(graph.configure({{"source","Source"},{"sum","Sum"},{"master","Master"}},
                           {{"source","sum",SendTap::PreFader,0.5F},
                            {"sum","master",SendTap::PostFader,1.0F}}, graph_error));

    const std::vector<BusAudioInput> inputs{
        {"master", {{0.0F, 0.0F}, {0.0F, 0.0F}}, 1.0F},
        {"source", {{1.0F, -1.0F}, {0.5F, -0.5F}}, 0.25F},
        {"sum", {{0.1F, 0.2F}, {0.2F, 0.4F}}, 2.0F}
    };
    std::vector<RenderedBus> rendered;
    BusRenderError error;
    assert(render_bus_graph(graph, inputs, rendered, error));
    assert(rendered.size() == 3);

    const auto find_bus = [&](const char* id) -> const RenderedBus& {
        for (const auto& bus : rendered) if (bus.bus_id == id) return bus;
        assert(false && "expected bus missing");
        return rendered.front();
    };
    const auto& source = find_bus("source");
    assert(std::abs(source.pre_fader.left[0] - 1.0F) < 1.0e-6F);
    assert(std::abs(source.post_fader.left[0] - 0.25F) < 1.0e-6F);

    // The source's pre-fader send adds 0.5 to Sum's direct 0.1; Sum fader then doubles it.
    const auto& sum = find_bus("sum");
    assert(std::abs(sum.pre_fader.left[0] - 0.6F) < 1.0e-6F);
    assert(std::abs(sum.post_fader.left[0] - 1.2F) < 1.0e-6F);
    // The post-fader send from Sum reaches Master after the Sum fader.
    const auto& master = find_bus("master");
    assert(std::abs(master.pre_fader.left[0] - 1.2F) < 1.0e-6F);
    assert(std::abs(master.post_fader.left[0] - 1.2F) < 1.0e-6F);

    auto bad_inputs = inputs;
    bad_inputs[0].audio.right.pop_back();
    assert(!render_bus_graph(graph, bad_inputs, rendered, error));
    assert(error.code == "bus_channel_length_mismatch");

    bad_inputs = inputs;
    bad_inputs.pop_back();
    assert(!render_bus_graph(graph, bad_inputs, rendered, error));
    assert(error.code == "bus_input_count");
    return 0;
}
