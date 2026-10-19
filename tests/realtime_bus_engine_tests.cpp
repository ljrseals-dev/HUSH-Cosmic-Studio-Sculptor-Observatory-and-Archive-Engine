#include "hush/realtime_bus_engine.hpp"

#include <cassert>
#include <cmath>

int main() {
    using namespace hush;
    RoutingError route_error;
    BusGraph graph;
    assert(graph.configure({{"source", "Source"}, {"master", "Master"}},
                           {{"source", "master", SendTap::PostFader, 1.0F}},
                           route_error));

    RealtimeBusEngine engine;
    ProcessorSetupError setup_error;
    assert(engine.configure(graph, 16, setup_error));
    assert(engine.bus_count() == 2);
    // This is a diagnostic only; strict lock-free status must be checked on target hardware.
    (void)engine.parameter_atomics_are_lock_free();

    const float source_l[] = {1.0F, 2.0F, -1.0F};
    const float source_r[] = {0.5F, 1.0F, -0.5F};
    const float master_l[] = {0.0F, 0.0F, 0.0F};
    const float master_r[] = {0.0F, 0.0F, 0.0F};
    const BusInputView inputs[] = {
        {source_l, source_r, 1.0F},
        {master_l, master_r, 1.0F}
    };

    // Updates are queued from the control side and applied by the next process call.
    assert(engine.enqueue_fader_change(0, 0.5F));
    assert(engine.process(inputs, 2, 3) == RealtimeProcessStatus::Ok);
    auto source = engine.post_fader(0, 3);
    auto master = engine.pre_fader(1, 3);
    assert(std::abs(source.left[0] - 0.5F) < 1.0e-6F);
    assert(std::abs(source.left[1] - 1.0F) < 1.0e-6F);
    assert(std::abs(master.left[0] - 0.5F) < 1.0e-6F);
    assert(std::abs(master.right[1] - 0.5F) < 1.0e-6F);

    // Latest queued updates are applied in FIFO order; final update wins for a bus.
    assert(engine.enqueue_fader_change(0, 0.25F));
    assert(engine.enqueue_fader_change(0, 0.75F));
    assert(engine.process(inputs, 2, 1) == RealtimeProcessStatus::Ok);
    source = engine.post_fader(0, 1);
    assert(std::abs(source.left[0] - 0.75F) < 1.0e-6F);

    assert(!engine.enqueue_fader_change(99, 1.0F));
    assert(!engine.enqueue_fader_change(0, -1.0F));
    assert(!engine.enqueue_fader_change(0, 17.0F));

    const BusInputView invalid[] = {
        {nullptr, source_r, 1.0F},
        {master_l, master_r, 1.0F}
    };
    assert(engine.process(invalid, 2, 1) == RealtimeProcessStatus::NullChannel);

    // Fixed capacity is bounded and full-queue behavior is explicit.
    std::size_t accepted = 0;
    while (engine.enqueue_fader_change(0, 1.0F)) ++accepted;
    assert(accepted == RealtimeBusEngine::kParameterQueueCapacity);
    assert(!engine.enqueue_fader_change(0, 1.0F));
    assert(engine.process(inputs, 2, 1) == RealtimeProcessStatus::Ok);
    return 0;
}
