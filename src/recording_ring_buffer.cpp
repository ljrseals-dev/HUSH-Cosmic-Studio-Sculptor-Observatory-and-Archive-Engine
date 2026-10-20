#include "hush/recording_ring_buffer.hpp"

#include <limits>
#include <new>

namespace hush {

bool RecordingRingBuffer::configure(std::size_t capacity_frames,
                                    std::uint16_t channels) noexcept {
    if (capacity_frames == 0U || channels == 0U || channels > 8U) return false;
    if (capacity_frames > std::numeric_limits<std::size_t>::max() / channels) return false;

    try {
        std::vector<float> replacement(capacity_frames * channels, 0.0F);
        samples_.swap(replacement);
    } catch (...) {
        return false;
    }

    capacity_frames_ = capacity_frames;
    channels_ = channels;
    write_frame_.store(0U, std::memory_order_relaxed);
    read_frame_.store(0U, std::memory_order_relaxed);
    dropped_frames_.store(0U, std::memory_order_relaxed);
    return true;
}

std::size_t RecordingRingBuffer::capture(std::span<const float> interleaved) noexcept {
    if (!configured() || interleaved.size() % channels_ != 0U) return 0U;
    const std::size_t input_frames = interleaved.size() / channels_;
    const std::uint64_t write = write_frame_.load(std::memory_order_relaxed);
    const std::uint64_t read = read_frame_.load(std::memory_order_acquire);
    const std::uint64_t used = write - read;
    if (used > capacity_frames_) return 0U;

    const std::size_t free_frames = capacity_frames_ - static_cast<std::size_t>(used);
    const std::size_t accepted = input_frames < free_frames ? input_frames : free_frames;
    for (std::size_t frame = 0; frame < accepted; ++frame) {
        const std::size_t slot = static_cast<std::size_t>((write + frame) % capacity_frames_);
        const std::size_t destination = slot * channels_;
        const std::size_t source = frame * channels_;
        for (std::size_t channel = 0; channel < channels_; ++channel)
            samples_[destination + channel] = interleaved[source + channel];
    }
    write_frame_.store(write + accepted, std::memory_order_release);
    if (accepted < input_frames)
        dropped_frames_.fetch_add(static_cast<std::uint64_t>(input_frames - accepted),
                                  std::memory_order_relaxed);
    return accepted;
}

std::size_t RecordingRingBuffer::drain(std::span<float> output) noexcept {
    if (!configured() || output.size() < channels_) return 0U;
    const std::size_t output_frames = output.size() / channels_;
    const std::uint64_t read = read_frame_.load(std::memory_order_relaxed);
    const std::uint64_t write = write_frame_.load(std::memory_order_acquire);
    const std::uint64_t available = write - read;
    if (available > capacity_frames_) return 0U;

    const std::size_t count = available < output_frames
        ? static_cast<std::size_t>(available) : output_frames;
    for (std::size_t frame = 0; frame < count; ++frame) {
        const std::size_t slot = static_cast<std::size_t>((read + frame) % capacity_frames_);
        const std::size_t source = slot * channels_;
        const std::size_t destination = frame * channels_;
        for (std::size_t channel = 0; channel < channels_; ++channel)
            output[destination + channel] = samples_[source + channel];
    }
    read_frame_.store(read + count, std::memory_order_release);
    return count;
}

std::size_t RecordingRingBuffer::frames_available() const noexcept {
    const std::uint64_t write = write_frame_.load(std::memory_order_acquire);
    const std::uint64_t read = read_frame_.load(std::memory_order_acquire);
    const std::uint64_t available = write - read;
    return available > capacity_frames_ ? 0U : static_cast<std::size_t>(available);
}

std::size_t RecordingRingBuffer::frames_free() const noexcept {
    return capacity_frames_ - frames_available();
}

} // namespace hush
