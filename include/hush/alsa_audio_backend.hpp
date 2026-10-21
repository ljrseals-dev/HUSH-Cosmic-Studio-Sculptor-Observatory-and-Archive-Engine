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
 enum class WorkerFailure : std::uint8_t {
  None = 0,
  CaptureRecoveryFailed,
  PlaybackRecoveryFailed
 };
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
 [[nodiscard]] std::uint64_t callback_count() const noexcept { return callbacks_.load(std::memory_order_relaxed); }
 [[nodiscard]] WorkerFailure worker_failure() const noexcept {
  return static_cast<WorkerFailure>(worker_failure_.load(std::memory_order_acquire));
 }
 // Raw negative ALSA status returned by snd_pcm_recover; zero means no terminal failure.
 [[nodiscard]] int worker_error_code() const noexcept { return worker_error_code_.load(std::memory_order_relaxed); }
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
 std::atomic<std::uint64_t> callbacks_{0};
 std::atomic<std::uint8_t> worker_failure_{static_cast<std::uint8_t>(WorkerFailure::None)};
 std::atomic<int> worker_error_code_{0};
 unsigned int actual_sample_rate_{0};
 std::size_t actual_period_frames_{0};
};
} // namespace hush
