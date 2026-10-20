#pragma once

#include "hush/spsc_parameter_queue.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace hush {

enum class TransportState : std::uint8_t { Stopped, Playing, Paused };
enum class TransportCommandKind : std::uint8_t { Play, Stop, Pause, SeekSamples };

struct TransportCommand {
    TransportCommandKind kind{TransportCommandKind::Stop};
    std::uint64_t sample_position{};
};

// Single control-thread producer and single audio-thread consumer. Call
// process_block() exactly once for each audio block; it never allocates or locks.
class TransportEngine {
public:
    static constexpr std::size_t kCommandCapacity = 256;

    [[nodiscard]] bool enqueue_play() noexcept;
    [[nodiscard]] bool enqueue_stop() noexcept;
    [[nodiscard]] bool enqueue_pause() noexcept;
    [[nodiscard]] bool enqueue_seek(std::uint64_t sample_position) noexcept;

    // Applies queued commands in FIFO order. Returns the position at block start.
    [[nodiscard]] std::uint64_t process_block(std::uint32_t frames) noexcept;

    [[nodiscard]] TransportState state() const noexcept {
        return state_.load(std::memory_order_acquire);
    }
    [[nodiscard]] std::uint64_t sample_position() const noexcept {
        return sample_position_.load(std::memory_order_acquire);
    }
    [[nodiscard]] bool commands_pending_approx() const noexcept {
        return commands_.size_approx() != 0;
    }

private:
    SpscParameterQueue<TransportCommand, kCommandCapacity> commands_;
    std::atomic<TransportState> state_{TransportState::Stopped};
    std::atomic<std::uint64_t> sample_position_{0};
};

struct TempoPoint {
    double beat{};
    double bpm{120.0};
};

// Fixed-capacity tempo map. Configure on a control thread and do not mutate it
// concurrently with readers. Tempo points must start at beat zero and be ordered.
class TempoMap {
public:
    static constexpr std::size_t kMaxPoints = 256;

    explicit TempoMap(double sample_rate = 48000.0) noexcept;
    [[nodiscard]] bool set_sample_rate(double sample_rate) noexcept;
    [[nodiscard]] bool set_points(const TempoPoint* points, std::size_t count) noexcept;
    [[nodiscard]] double sample_to_beat(std::uint64_t sample_position) const noexcept;
    [[nodiscard]] bool beat_to_sample(double beat, std::uint64_t& sample_position) const noexcept;
    [[nodiscard]] std::size_t point_count() const noexcept { return count_; }
    [[nodiscard]] double sample_rate() const noexcept { return sample_rate_; }

private:
    double sample_rate_{48000.0};
    std::array<TempoPoint, kMaxPoints> points_{{TempoPoint{0.0, 120.0}}};
    std::size_t count_{1};
};

} // namespace hush
