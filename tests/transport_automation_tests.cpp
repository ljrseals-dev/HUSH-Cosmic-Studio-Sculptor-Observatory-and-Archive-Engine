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

    // A seek to the maximum sample position must saturate rather than wrap.
    const auto max_sample = std::numeric_limits<std::uint64_t>::max();
    assert(transport.enqueue_seek(max_sample));
    assert(transport.enqueue_play());
    assert(transport.process_block(128) == max_sample);
    assert(transport.sample_position() == max_sample);
    assert(transport.state() == TransportState::Playing);

    // Queue commands are applied in FIFO order, including seek/stop/seek/play.
    TransportEngine ordered;
    assert(ordered.enqueue_seek(100));
    assert(ordered.enqueue_stop());
    assert(ordered.enqueue_seek(25));
    assert(ordered.enqueue_play());
    assert(ordered.process_block(4) == 25);
    assert(ordered.state() == TransportState::Playing);
    assert(ordered.sample_position() == 29);

    // A full queue rejects a new command without blocking or overwriting older
    // commands. Draining it restores capacity for subsequent producer commands.
    TransportEngine bounded;
    for (std::size_t i = 0; i < TransportEngine::kCommandCapacity; ++i)
        assert(bounded.enqueue_play());
    assert(!bounded.enqueue_stop());
    assert(bounded.commands_pending_approx());
    (void)bounded.process_block(0);
    assert(!bounded.commands_pending_approx());
    assert(bounded.enqueue_stop());
    (void)bounded.process_block(0);
    assert(bounded.state() == TransportState::Stopped);

    TempoMap tempo(48000.0);
    std::uint64_t sample = 0;

    // Sample-rate validation accepts the documented inclusive endpoints and
    // rejects out-of-range values without changing the previous configuration.
    assert(!tempo.set_sample_rate(7999.0));
    assert(tempo.sample_rate() == 48000.0);
    assert(!tempo.set_sample_rate(768001.0));
    assert(tempo.sample_rate() == 48000.0);
    assert(tempo.set_sample_rate(8000.0));
    assert(tempo.sample_rate() == 8000.0);
    assert(tempo.set_sample_rate(768000.0));
    assert(tempo.sample_rate() == 768000.0);
    assert(tempo.set_sample_rate(48000.0));

    // A tempo map is bounded, requires at least one point, and rejects null
    // input. Invalid replacements must not disturb the accepted default map.
    assert(!tempo.set_points(nullptr, 1));
    assert(!tempo.set_points(nullptr, 0));
    TempoPoint too_many_points[TempoMap::kMaxPoints + 1]{};
    for (std::size_t i = 0; i < TempoMap::kMaxPoints + 1; ++i)
        too_many_points[i] = {static_cast<double>(i), 120.0};
    assert(!tempo.set_points(too_many_points, TempoMap::kMaxPoints + 1));
    assert(tempo.point_count() == 1);
    assert(tempo.beat_to_sample(4.0, sample) && sample == 96000);
    assert(std::abs(tempo.sample_to_beat(96000) - 4.0) < 1.0e-9);

    // uint64_t positions above INT64_MAX are valid; do not route them through
    // llround(), whose return type is only long long.
    assert(tempo.beat_to_sample(1.0e19 / 24000.0, sample));
    assert(sample > static_cast<std::uint64_t>(
        std::numeric_limits<long long>::max()));
    assert(sample < max_sample);

    // Out-of-range conversion must fail without overwriting the caller's value.
    sample = 12345;
    assert(!tempo.beat_to_sample(1.0e20 / 24000.0, sample));
    assert(sample == 12345);

    const TempoPoint points[] = {{0.0, 120.0}, {4.0, 60.0}};
    assert(tempo.set_points(points, 2));
    assert(tempo.beat_to_sample(4.0, sample) && sample == 96000);
    assert(tempo.beat_to_sample(5.0, sample) && sample == 144000);
    assert(std::abs(tempo.sample_to_beat(144000) - 5.0) < 1.0e-9);
    const TempoPoint invalid[] = {{0.0, 120.0}, {0.0, 90.0}};
    assert(!tempo.set_points(invalid, 2));
    assert(!tempo.beat_to_sample(-1.0, sample));
    assert(!tempo.set_sample_rate(std::numeric_limits<double>::quiet_NaN()));
    // Rejected edits preserve the last valid tempo map and sample rate.
    assert(tempo.sample_rate() == 48000.0);
    assert(tempo.beat_to_sample(5.0, sample) && sample == 144000);
    const TempoPoint non_finite[] = {
        {0.0, 120.0},
        {4.0, std::numeric_limits<double>::infinity()}
    };
    assert(!tempo.set_points(non_finite, 2));
    assert(tempo.beat_to_sample(5.0, sample) && sample == 144000);

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
    // Invalid replacement is transactional: the accepted lane remains intact.
    assert(step.point_count() == 2);
    assert(step.evaluate(1.0) == 0.25F);
    assert(!step.set_points({{0.0, std::numeric_limits<float>::quiet_NaN()}},
                            AutomationInterpolation::Linear));
    assert(!step.set_points({{0.0, 0.5F}},
                            static_cast<AutomationInterpolation>(255)));
    assert(step.point_count() == 2);
    assert(step.evaluate(std::numeric_limits<double>::quiet_NaN()) == 0.0F);
    return 0;
}
