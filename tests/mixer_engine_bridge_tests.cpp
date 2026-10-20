#include "hush/mixer_engine_bridge.hpp"

#include <cassert>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

int main() {
    using namespace hush;

    RoutingError route_error;
    BusGraph graph;
    assert(graph.configure({{"track-bus", "Track Bus"}, {"master", "Master"}},
                           {{"track-bus", "master", SendTap::PostFader, 1.0F}},
                           route_error));

    RealtimeBusEngine engine;
    ProcessorSetupError setup_error;
    assert(engine.configure(graph, 8, setup_error));

    MixerEngineBridge bridge;
    std::string error;
    assert(bridge.configure(engine, {{"track-1", 0}}, error));
    assert(bridge.ready());
    assert(error.empty());

    // Accepted control-side changes are applied at the next process/block boundary.
    assert(bridge.enqueue_gain("track-1", 0.25F));
    assert(!bridge.enqueue_gain("missing-track", 1.0F));
    assert(!bridge.enqueue_gain("track-1", -1.0F));
    assert(!bridge.enqueue_gain("track-1", 17.0F));
    assert(!bridge.enqueue_gain("track-1", std::nanf("")));

    const float input_l[] = {1.0F, 2.0F};
    const float input_r[] = {0.5F, 1.0F};
    const float master_l[] = {0.0F, 0.0F};
    const float master_r[] = {0.0F, 0.0F};
    const BusAudioInputView inputs[] = {{input_l, input_r}, {master_l, master_r}};
    assert(engine.process(inputs, 2, 2) == RealtimeProcessStatus::Ok);

    const auto post = engine.post_fader(0, 2);
    assert(std::abs(post.left[0] - 0.25F) < 1.0e-6F);
    assert(std::abs(post.left[1] - 0.50F) < 1.0e-6F);
    assert(std::abs(post.right[0] - 0.125F) < 1.0e-6F);
    assert(std::abs(post.right[1] - 0.25F) < 1.0e-6F);

    // Bad mappings are rejected without making an unconfigured bridge ready.
    MixerEngineBridge invalid;
    assert(!invalid.configure(engine, {{"track-1", 0}, {"track-1", 1}}, error));
    assert(!invalid.ready());
    assert(!invalid.configure(engine, {{"track-2", 99}}, error));
    assert(!invalid.configure(engine, {{"", 0}}, error));

    // Empty mapping is valid but maps no tracks.
    MixerEngineBridge empty;
    assert(empty.configure(engine, {}, error));
    assert(empty.ready());
    assert(!empty.enqueue_gain("track-1", 0.5F));
    return 0;
}
