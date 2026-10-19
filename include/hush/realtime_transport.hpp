#pragma once

#include "hush/spsc_parameter_queue.hpp"

#include <cstddef>
#include <cstdint>

namespace hush {

enum class TransportCommandKind : std::uint8_t { Play, Stop, Seek };

struct TransportCommand {
    TransportCommandKind kind{TransportCommandKind::Stop};
    std::uint64_t position_samples{};
};

// Fixed-capacity, block-boundary transport state. The control/UI thread is the
// sole producer; the audio processing thread is the sole consumer. This is an
// internal sample counter, not an external clock or sample-scheduled event queue.
template <std::size_t QueueCapacity = 64>
class RealtimeTransport {
public:
    [[nodiscard]] bool enqueue_play() noexcept {
        return commands_.try_push({TransportCommandKind::Play, 0});
    }
    [[nodiscard]] bool enqueue_stop() noexcept {
        return commands_.try_push({TransportCommandKind::Stop, 0});
    }
    [[nodiscard]] bool enqueue_seek(std::uint64_t position_samples) noexcept {
        return commands_.try_push({TransportCommandKind::Seek, position_samples});
    }

    // Call exactly once per audio block. Commands take effect at this boundary;
    // the returned position is the first sample position of the current block.
    [[nodiscard]] std::uint64_t begin_block(std::size_t frame_count) noexcept {
        TransportCommand command{};
        while (commands_.try_pop(command)) {
            switch (command.kind) {
            case TransportCommandKind::Play: playing_ = true; break;
            case TransportCommandKind::Stop: playing_ = false; break;
            case TransportCommandKind::Seek: position_samples_ = command.position_samples; break;
            }
        }
        const auto block_start = position_samples_;
        if (playing_) position_samples_ += static_cast<std::uint64_t>(frame_count);
        return block_start;
    }

    [[nodiscard]] bool is_playing() const noexcept { return playing_; }
    [[nodiscard]] std::uint64_t position_samples() const noexcept { return position_samples_; }
    [[nodiscard]] bool queue_atomics_are_lock_free() const noexcept {
        return commands_.atomics_are_lock_free();
    }

private:
    SpscParameterQueue<TransportCommand, QueueCapacity> commands_;
    std::uint64_t position_samples_{0};
    bool playing_{false};
};

} // namespace hush
