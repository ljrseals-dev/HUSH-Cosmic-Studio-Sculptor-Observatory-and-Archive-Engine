#pragma once
#include "hush/recording_ring_buffer.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
namespace hush {
// One SPSC capture producer and one disk-writer worker. Start/stop are control-thread calls.
// capture() performs no allocation, locking, logging, or file I/O.
class RecordingSession {
public:
 RecordingSession()=default; RecordingSession(const RecordingSession&)=delete; RecordingSession& operator=(const RecordingSession&)=delete; ~RecordingSession();
 [[nodiscard]] bool start(const std::filesystem::path&,std::uint32_t,std::uint16_t,std::size_t,std::size_t=2048U,std::string* = nullptr);
 [[nodiscard]] std::size_t capture(std::span<const float>) noexcept;
 [[nodiscard]] bool stop(std::string* = nullptr);
 [[nodiscard]] bool running() const noexcept{return accepting_.load(std::memory_order_acquire);}
 [[nodiscard]] std::uint64_t dropped_frames() const noexcept{return ring_.dropped_frames();}
 [[nodiscard]] std::uint64_t frames_written() const noexcept{return frames_written_.load(std::memory_order_acquire);}
 [[nodiscard]] std::string last_error() const;
private:
 void worker_main(std::filesystem::path) noexcept;
 RecordingRingBuffer ring_;std::vector<float> scratch_;std::thread worker_;
 mutable std::mutex mutex_;std::condition_variable wake_;std::string error_;
 std::atomic<bool> accepting_{false},stopping_{false},ready_{false},opened_{false},worker_failed_{false};
 std::atomic<std::uint64_t> frames_written_{0};std::uint32_t sample_rate_{0};std::uint16_t channels_{0};
};
}