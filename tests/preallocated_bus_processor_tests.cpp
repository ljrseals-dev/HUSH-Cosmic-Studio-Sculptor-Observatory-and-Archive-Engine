#include "hush/preallocated_bus_processor.hpp"

#include <cassert>
#include <cmath>

int main() {
    using namespace hush;
    RoutingError routing_error;
    BusGraph graph;
    assert(graph.configure({{"source", "Source"}, {"sum", "Sum"}, {"master", "Master"}},
                           {{"source", "sum", SendTap::PreFader, 0.5F},
                            {"sum", "master", SendTap::PostFader, 1.0F}},
                           routing_error));

    PreallocatedBusProcessor processor;
    ProcessorSetupError setup_error;
    assert(processor.configure(graph, 8, setup_error));
    assert(processor.bus_count() == 3);
    assert(processor.max_frames() == 8);

    const float source_l[] = {1.0F, -1.0F};
    const float source_r[] = {0.5F, -0.5F};
    const float sum_l[] = {0.1F, 0.2F};
    const float sum_r[] = {0.2F, 0.4F};
    const float master_l[] = {0.0F, 0.0F};
    const float master_r[] = {0.0F, 0.0F};
    const BusInputView inputs[] = {
        {source_l, source_r, 0.25F},
        {sum_l, sum_r, 2.0F},
        {master_l, master_r, 1.0F}
    };
    assert(processor.process(inputs, 3, 2) == RealtimeProcessStatus::Ok);

    const auto source_post = processor.post_fader(0, 2);
    assert(std::abs(source_post.left[0] - 0.25F) < 1.0e-6F);
    const auto sum_pre = processor.pre_fader(1, 2);
    const auto sum_post = processor.post_fader(1, 2);
    assert(std::abs(sum_pre.left[0] - 0.6F) < 1.0e-6F);
    assert(std::abs(sum_post.left[0] - 1.2F) < 1.0e-6F);
    const auto master_pre = processor.pre_fader(2, 2);
    assert(std::abs(master_pre.left[0] - 1.2F) < 1.0e-6F);

    assert(processor.process(inputs, 3, 9) == RealtimeProcessStatus::FrameCountExceedsCapacity);
    const BusInputView bad_gain[] = {
        {source_l, source_r, 1.0F},
        {sum_l, sum_r, 100.0F},
        {master_l, master_r, 1.0F}
    };
    assert(processor.process(bad_gain, 3, 2) == RealtimeProcessStatus::InvalidFaderGain);
    const BusInputView null_channel[] = {
        {nullptr, source_r, 1.0F},
        {sum_l, sum_r, 1.0F},
        {master_l, master_r, 1.0F}
    };
    assert(processor.process(null_channel, 3, 2) == RealtimeProcessStatus::NullChannel);
    return 0;
}
