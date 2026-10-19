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

// Fixed-capacity GUI/control -> audio-thread transport. Call process_block() once
// per audio block. It performs no allocation, locking, logging, or I/O.
class TransportEngine {
public:
    static constexpr std::size_t kCommandCapacity = 256;

    [[nodiscard]] bool enqueue_play() noexcept;
    [[nodiscard]] bool enqueue_stop() noexcept;
    [[nodiscard]] bool enqueue_pause() noexcept;
    [[nodiscard]] bool enqueue_seek(std::uint64_t sample_position) noexcept;

    // Audio-thread only. Applies pending commands in FIFO order and advances the
    // sample clock only while Playing. Returns the position at block start.
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

// Immutable-during-playback tempo map. Build/edit on the control thread before
// publishing it to audio/UI readers. Points must be strictly beat-ordered.
// Conversion functions are allocation-free; sample positions are rounded to the
// nearest sample. A tempo point changes tempo at its stated beat.
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
