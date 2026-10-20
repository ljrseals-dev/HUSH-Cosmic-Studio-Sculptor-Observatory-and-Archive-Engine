#include "hush/automation_lane.hpp"
#include "hush/transport_engine.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>

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

    TempoMap tempo(48000.0);
    std::uint64_t sample = 0;
    assert(tempo.beat_to_sample(4.0, sample) && sample == 96000);
    assert(std::abs(tempo.sample_to_beat(96000) - 4.0) < 1.0e-9);
    const TempoPoint points[] = {{0.0, 120.0}, {4.0, 60.0}};
    assert(tempo.set_points(points, 2));
    assert(tempo.beat_to_sample(4.0, sample) && sample == 96000);
    assert(tempo.beat_to_sample(5.0, sample) && sample == 144000);
    assert(std::abs(tempo.sample_to_beat(144000) - 5.0) < 1.0e-9);
    const TempoPoint invalid[] = {{0.0, 120.0}, {0.0, 90.0}};
    assert(!tempo.set_points(invalid, 2));
    assert(!tempo.beat_to_sample(-1.0, sample));
    assert(!tempo.set_sample_rate(std::numeric_limits<double>::quiet_NaN()));

    AutomationLane linear;
    assert(linear.set_points({{0.0, 0.0F}, {4.0, 1.0F}},
                             AutomationInterpolation::Linear));
    assert(std::abs(linear.evaluate(2.0) - 0.5F) < 1.0e-6F);
    assert(linear.evaluate(-1.0) == 0.0F);
    assert(linear.evaluate(5.0) == 1.0F);
    assert(std::abs(linear.evaluate(4.0) - 1.0F) < 1.0e-6F);

    AutomationLane step;
    assert(step.set_points({{0.0, 0.25F}, {2.0, 0.75F}},
                           AutomationInterpolation::Step));
    assert(step.evaluate(1.999) == 0.25F);
    assert(step.evaluate(2.0) == 0.75F);
    assert(!step.set_points({{0.0, 0.0F}, {0.0, 1.0F}},
                            AutomationInterpolation::Linear));
    assert(step.evaluate(std::numeric_limits<double>::quiet_NaN()) == 0.0F);
    return 0;
}
