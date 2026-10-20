#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace hush {

// Single-producer/single-consumer interleaved float ring for recording capture.
// Configure on the control thread before starting audio. capture() is the sole
// producer operation and drain() is the sole consumer operation. Neither
// operation allocates, locks, logs, or performs I/O.
class RecordingRingBuffer {
public:
    RecordingRingBuffer() = default;
    RecordingRingBuffer(const RecordingRingBuffer&) = delete;
    RecordingRingBuffer& operator=(const RecordingRingBuffer&) = delete;

    // Allocates storage on the control thread. Call only while producer and
    // consumer are stopped. Supports 1–8 channels and any positive capacity.
    [[nodiscard]] bool configure(std::size_t capacity_frames, std::uint16_t channels) noexcept;

    // Called by the capture/audio thread with whole interleaved frames.
    // Returns accepted frames. If full, remaining input frames are dropped and
    // counted; the callback never waits for the consumer.
    [[nodiscard]] std::size_t capture(std::span<const float> interleaved) noexcept;

    // Called by a non-realtime consumer, e.g. the recording disk-writer thread.
    // Copies up to output.size()/channels complete frames. Returns frames read.
    [[nodiscard]] std::size_t drain(std::span<float> output) noexcept;

    [[nodiscard]] std::size_t capacity_frames() const noexcept { return capacity_frames_; }
    [[nodiscard]] std::uint16_t channels() const noexcept { return channels_; }
    [[nodiscard]] std::size_t frames_available() const noexcept;
    [[nodiscard]] std::size_t frames_free() const noexcept;
    [[nodiscard]] std::uint64_t dropped_frames() const noexcept {
        return dropped_frames_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] bool configured() const noexcept { return capacity_frames_ > 0 && channels_ > 0; }

private:
    std::vector<float> samples_;
    std::size_t capacity_frames_{0};
    std::uint16_t channels_{0};
    alignas(64) std::atomic<std::uint64_t> write_frame_{0};
    alignas(64) std::atomic<std::uint64_t> read_frame_{0};
    std::atomic<std::uint64_t> dropped_frames_{0};
};

} // namespace hush
