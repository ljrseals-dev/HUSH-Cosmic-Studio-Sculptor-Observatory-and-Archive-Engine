#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
namespace hush {
// Single-producer/single-consumer ring for interleaved float audio. Construct before opening device.
class RecordingRingBuffer {
public:
 RecordingRingBuffer(std::uint16_t channels,std::size_t capacity_frames);
 RecordingRingBuffer(const RecordingRingBuffer&)=delete;
 RecordingRingBuffer& operator=(const RecordingRingBuffer&)=delete;
 [[nodiscard]] bool valid()const noexcept{return channels_>0&&capacity_frames_>0&&!samples_.empty();}
 [[nodiscard]] std::uint16_t channels()const noexcept{return channels_;}
 [[nodiscard]] std::size_t capacity_frames()const noexcept{return capacity_frames_;}
 [[nodiscard]] std::size_t available_frames()const noexcept;
 [[nodiscard]] std::uint64_t dropped_frames()const noexcept{return dropped_frames_.load(std::memory_order_relaxed);}
 // Audio producer: no allocation, locks, logging, or I/O. Rejects whole blocks on overflow.
 [[nodiscard]] bool push(std::span<const float>)noexcept;
 // Worker consumer copies into caller-owned storage.
 [[nodiscard]] std::size_t pop(std::span<float>)noexcept;
private:
 const std::uint16_t channels_;const std::size_t capacity_frames_;std::vector<float> samples_;
 alignas(64) std::atomic<std::uint64_t> write_frame_{0};alignas(64) std::atomic<std::uint64_t> read_frame_{0};
 std::atomic<std::uint64_t> dropped_frames_{0};
};
} // namespace hush
