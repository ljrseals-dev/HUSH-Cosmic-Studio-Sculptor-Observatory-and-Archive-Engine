#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>
struct _snd_pcm;
typedef struct _snd_pcm snd_pcm_t;
namespace hush {
class AlsaAudioBackend {
public:
 using ProcessCallback = void (*)(const float*, float*, std::size_t, void*) noexcept;
 struct Config {
  std::string capture_device{"default"};
  std::string playback_device{"default"};
  unsigned int sample_rate{48000};
  std::size_t period_frames{128};
 };
 AlsaAudioBackend() = default;
 ~AlsaAudioBackend();
 AlsaAudioBackend(const AlsaAudioBackend&) = delete;
 AlsaAudioBackend& operator=(const AlsaAudioBackend&) = delete;
 [[nodiscard]] bool start(const Config&, ProcessCallback, void*, std::string& error);
 void stop() noexcept;
 [[nodiscard]] bool running() const noexcept { return running_.load(std::memory_order_acquire); }
 [[nodiscard]] std::uint64_t xrun_count() const noexcept { return xruns_.load(std::memory_order_relaxed); }
 [[nodiscard]] std::uint64_t recovery_count() const noexcept { return recoveries_.load(std::memory_order_relaxed); }
 [[nodiscard]] std::uint64_t short_transfer_count() const noexcept { return short_transfers_.load(std::memory_order_relaxed); }
 [[nodiscard]] std::uint64_t callback_count() const noexcept { return callbacks_.load(std::memory_order_relaxed); }
 [[nodiscard]] unsigned int actual_sample_rate() const noexcept { return actual_sample_rate_; }
 [[nodiscard]] std::size_t actual_period_frames() const noexcept { return actual_period_frames_; }
private:
 void run() noexcept;
 void close_devices() noexcept;
 snd_pcm_t* capture_{nullptr};
 snd_pcm_t* playback_{nullptr};
 ProcessCallback callback_{nullptr};
 void* user_data_{nullptr};
 std::vector<float> input_buffer_;
 std::vector<float> output_buffer_;
 std::thread worker_;
 std::atomic<bool> stop_requested_{false};
 std::atomic<bool> running_{false};
 std::atomic<std::uint64_t> xruns_{0};
 std::atomic<std::uint64_t> recoveries_{0};
 std::atomic<std::uint64_t> short_transfers_{0};
 std::atomic<std::uint64_t> callbacks_{0};
 unsigned int actual_sample_rate_{0};
 std::size_t actual_period_frames_{0};
};
} // namespace hush
