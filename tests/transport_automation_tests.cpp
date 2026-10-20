#include "hush/automation_lane.hpp"
#include "hush/transport_engine.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

int main() {
    using namespace hush;

    TransportEngine transport;
    assert(transport.state() == TransportState::Stopped);
    assert(transport.process_block(128) == 0);
    assert(transport.sample_position() == 0);
    assert(transport.enqueue_play());
    assert(transport.process_block(128) == 0);
    assert(transport.state() == TransportState::Playing);
    assert(transport.sample_position() == 128);
    assert(transport.enqueue_seek(48000));
    assert(transport.process_block(64) == 48000);
    assert(transport.sample_position() == 48064);
    assert(transport.enqueue_pause());
    assert(transport.process_block(64) == 48064);
    assert(transport.state() == TransportState::Paused);
    assert(transport.sample_position() == 48064);
    assert(transport.enqueue_play());
    assert(transport.process_block(64) == 48064);
    assert(transport.sample_position() == 48128);
    assert(transport.enqueue_stop());
    assert(transport.process_block(64) == 0);
    assert(transport.state() == TransportState::Stopped);

    // The fixed-capacity command queue must reject overflow without blocking.
    TransportEngine bounded;
    for (std::size_t i = 0; i < TransportEngine::kCommandCapacity; ++i) {
        assert(bounded.enqueue_play());
    }
    assert(!bounded.enqueue_stop());
    assert(bounded.commands_pending_approx());
    (void)bounded.process_block(0);
    assert(!bounded.commands_pending_approx());

    // Seeking to the largest representable position must not wrap the sample clock.
    assert(bounded.enqueue_seek(std::numeric_limits<std::uint64_t>::max()));
    assert(bounded.enqueue_play());
    assert(bounded.process_block(128) == std::numeric_limits<std::uint64_t>::max());
    assert(bounded.sample_position() == std::numeric_limits<std::uint64_t>::max());

    TempoMap tempo(48000.0);
    std::uint64_t sample = 0;
    assert(tempo.beat_to_sample(4.0, sample));
    assert(sample == 96000);
    assert(std::abs(tempo.sample_to_beat(96000) - 4.0) < 1.0e-9);
    // uint64_t sample positions above INT64_MAX are valid when still in range.
    assert(tempo.beat_to_sample(1.0e19 / 24000.0, sample));
    assert(sample > static_cast<std::uint64_t>(std::numeric_limits<long long>::max()));
    const TempoPoint points[] = {{0.0, 120.0}, {4.0, 60.0}};
    assert(tempo.set_points(points, 2));
    assert(tempo.beat_to_sample(4.0, sample) && sample == 96000);
    assert(tempo.beat_to_sample(5.0, sample) && sample == 144000);
    assert(std::abs(tempo.sample_to_beat(144000) - 5.0) < 1.0e-9);
    const TempoPoint invalid[] = {{0.0, 120.0}, {0.0, 90.0}};
    assert(!tempo.set_points(invalid, 2));
    assert(!tempo.beat_to_sample(-1.0, sample));
    assert(!tempo.set_sample_rate(std::numeric_limits<double>::quiet_NaN()));
    // Failed map edits are transactional: the last valid map remains usable.
    assert(tempo.beat_to_sample(5.0, sample) && sample == 144000);
    const TempoPoint non_finite[] = {{0.0, 120.0}, {4.0, std::numeric_limits<double>::infinity()}};
    assert(!tempo.set_points(non_finite, 2));
    assert(tempo.beat_to_sample(5.0, sample) && sample == 144000);

    AutomationLane linear;
    assert(linear.set_points({{0.0, 0.0F}, {4.0, 1.0F}}, AutomationInterpolation::Linear));
    assert(std::abs(linear.evaluate(2.0) - 0.5F) < 1.0e-6F);
    assert(linear.evaluate(-1.0) == 0.0F);
    assert(linear.evaluate(5.0) == 1.0F);
    assert(std::abs(linear.evaluate(4.0) - 1.0F) < 1.0e-6F);

    AutomationLane step;
    assert(step.set_points({{0.0, 0.25F}, {2.0, 0.75F}}, AutomationInterpolation::Step));
    assert(step.evaluate(1.999) == 0.25F);
    assert(step.evaluate(2.0) == 0.75F);
    assert(!step.set_points({{0.0, 0.0F}, {0.0, 1.0F}}, AutomationInterpolation::Linear));
    assert(!step.set_points({{0.0, 0.0F}}, static_cast<AutomationInterpolation>(255)));
    // A rejected edit must leave the previously accepted lane intact.
    assert(step.point_count() == 2);
    assert(step.evaluate(1.0) == 0.25F);
    assert(step.evaluate(std::numeric_limits<double>::quiet_NaN()) == 0.0F);
    return 0;
}
