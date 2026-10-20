#include "hush/transport_engine.hpp"

#include <cmath>
#include <limits>

namespace hush {
namespace {
constexpr double kSecondsPerMinute = 60.0;
constexpr double kMinBpm = 1.0;
constexpr double kMaxBpm = 1000.0;
constexpr double kMinSampleRate = 8000.0;
constexpr double kMaxSampleRate = 768000.0;
}

bool TransportEngine::enqueue_play() noexcept {
    return commands_.try_push({TransportCommandKind::Play, 0});
}
bool TransportEngine::enqueue_stop() noexcept {
    return commands_.try_push({TransportCommandKind::Stop, 0});
}
bool TransportEngine::enqueue_pause() noexcept {
    return commands_.try_push({TransportCommandKind::Pause, 0});
}
bool TransportEngine::enqueue_seek(std::uint64_t position) noexcept {
    return commands_.try_push({TransportCommandKind::SeekSamples, position});
}

std::uint64_t TransportEngine::process_block(std::uint32_t frames) noexcept {
    TransportCommand command{};
    while (commands_.try_pop(command)) {
        switch (command.kind) {
        case TransportCommandKind::Play:
            state_.store(TransportState::Playing, std::memory_order_release);
            break;
        case TransportCommandKind::Stop:
            state_.store(TransportState::Stopped, std::memory_order_release);
            sample_position_.store(0, std::memory_order_release);
            break;
        case TransportCommandKind::Pause:
            if (state_.load(std::memory_order_relaxed) == TransportState::Playing)
                state_.store(TransportState::Paused, std::memory_order_release);
            break;
        case TransportCommandKind::SeekSamples:
            sample_position_.store(command.sample_position, std::memory_order_release);
            break;
        }
    }

    const auto start = sample_position_.load(std::memory_order_relaxed);
    if (state_.load(std::memory_order_relaxed) == TransportState::Playing) {
        const auto max = std::numeric_limits<std::uint64_t>::max();
        const auto end = static_cast<std::uint64_t>(frames) > max - start
            ? max : start + static_cast<std::uint64_t>(frames);
        sample_position_.store(end, std::memory_order_release);
    }
    return start;
}

TempoMap::TempoMap(double sample_rate) noexcept {
    if (!set_sample_rate(sample_rate)) sample_rate_ = 48000.0;
}

bool TempoMap::set_sample_rate(double rate) noexcept {
    if (!std::isfinite(rate) || rate < kMinSampleRate || rate > kMaxSampleRate)
        return false;
    sample_rate_ = rate;
    return true;
}

bool TempoMap::set_points(const TempoPoint* points, std::size_t count) noexcept {
    if (points == nullptr || count == 0 || count > kMaxPoints ||
        points[0].beat != 0.0) return false;

    double previous = -1.0;
    for (std::size_t i = 0; i < count; ++i) {
        if (!std::isfinite(points[i].beat) || !std::isfinite(points[i].bpm) ||
            points[i].beat < 0.0 || points[i].beat <= previous ||
            points[i].bpm < kMinBpm || points[i].bpm > kMaxBpm) return false;
        previous = points[i].beat;
    }
    for (std::size_t i = 0; i < count; ++i) points_[i] = points[i];
    count_ = count;
    return true;
}

double TempoMap::sample_to_beat(std::uint64_t sample) const noexcept {
    const double target = static_cast<double>(sample);
    double accumulated_samples = 0.0;
    double current_beat = 0.0;

    for (std::size_t i = 0; i < count_; ++i) {
        const double next_beat = i + 1 < count_
            ? points_[i + 1].beat : std::numeric_limits<double>::infinity();
        const double samples_per_beat =
            sample_rate_ * kSecondsPerMinute / points_[i].bpm;
        const double segment_samples = (next_beat - current_beat) * samples_per_beat;
        if (target < accumulated_samples + segment_samples)
            return current_beat + (target - accumulated_samples) / samples_per_beat;
        accumulated_samples += segment_samples;
        current_beat = next_beat;
    }
    return current_beat;
}

bool TempoMap::beat_to_sample(double beat, std::uint64_t& sample) const noexcept {
    if (!std::isfinite(beat) || beat < 0.0) return false;
    double accumulated_samples = 0.0;
    double current_beat = 0.0;
    const double max_sample = static_cast<double>(
        std::numeric_limits<std::uint64_t>::max());

    for (std::size_t i = 0; i < count_; ++i) {
        const double next_beat = i + 1 < count_
            ? points_[i + 1].beat : std::numeric_limits<double>::infinity();
        const double samples_per_beat =
            sample_rate_ * kSecondsPerMinute / points_[i].bpm;
        if (beat < next_beat || i + 1 == count_) {
            const double value =
                accumulated_samples + (beat - current_beat) * samples_per_beat;
            if (!std::isfinite(value) || value < 0.0 || value >= max_sample)
                return false;
            sample = static_cast<std::uint64_t>(std::llround(value));
            return true;
        }
        accumulated_samples += (next_beat - current_beat) * samples_per_beat;
        current_beat = next_beat;
    }
    return false;
}

} // namespace hush
