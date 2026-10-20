#include "hush/alsa_audio_backend.hpp"
#include <alsa/asoundlib.h>
#include <algorithm>
#include <limits>
#include <string>
namespace hush {
namespace {
bool configure_pcm(snd_pcm_t* pcm, unsigned int requested_rate, std::size_t requested_period,
                   unsigned int& actual_rate, std::size_t& actual_period, std::string& error) {
 snd_pcm_hw_params_t* params = nullptr;
 snd_pcm_hw_params_alloca(&params);
 int rc = snd_pcm_hw_params_any(pcm, params);
 if (rc < 0) { error = snd_strerror(rc); return false; }
 if ((rc = snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0 ||
     (rc = snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_FLOAT_LE)) < 0 ||
     (rc = snd_pcm_hw_params_set_channels(pcm, params, 2)) < 0) {
  error = snd_strerror(rc); return false;
 }
 unsigned int rate = requested_rate;
 int direction = 0;
 if ((rc = snd_pcm_hw_params_set_rate_near(pcm, params, &rate, &direction)) < 0) {
  error = snd_strerror(rc); return false;
 }
 snd_pcm_uframes_t period = static_cast<snd_pcm_uframes_t>(requested_period);
 direction = 0;
 if ((rc = snd_pcm_hw_params_set_period_size_near(pcm, params, &period, &direction)) < 0) {
  error = snd_strerror(rc); return false;
 }
 if ((rc = snd_pcm_hw_params(pcm, params)) < 0) { error = snd_strerror(rc); return false; }
 actual_rate = rate; actual_period = static_cast<std::size_t>(period);
 return true;
}
}
AlsaAudioBackend::~AlsaAudioBackend() { stop(); }
bool AlsaAudioBackend::start(const Config& config, ProcessCallback callback, void* user_data, std::string& error) {
 stop();
 if (!callback || config.sample_rate < 8000 || config.sample_rate > 384000 ||
     config.period_frames == 0 ||
     config.period_frames > static_cast<std::size_t>(std::numeric_limits<snd_pcm_uframes_t>::max())) {
  error = "invalid_callback_or_audio_configuration"; return false;
 }
 int rc = snd_pcm_open(&capture_, config.capture_device.c_str(), SND_PCM_STREAM_CAPTURE, 0);
 if (rc < 0) { error = std::string("capture_open: ") + snd_strerror(rc); close_devices(); return false; }
 rc = snd_pcm_open(&playback_, config.playback_device.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
 if (rc < 0) { error = std::string("playback_open: ") + snd_strerror(rc); close_devices(); return false; }
 unsigned int capture_rate = 0, playback_rate = 0;
 std::size_t capture_period = 0, playback_period = 0;
 if (!configure_pcm(capture_, config.sample_rate, config.period_frames, capture_rate, capture_period, error) ||
     !configure_pcm(playback_, config.sample_rate, config.period_frames, playback_rate, playback_period, error)) {
  close_devices(); return false;
 }
 if (capture_rate != playback_rate || capture_period != playback_period) {
  error = "capture_playback_rate_or_period_mismatch"; close_devices(); return false;
 }
 if ((rc = snd_pcm_prepare(capture_)) < 0 || (rc = snd_pcm_prepare(playback_)) < 0) {
  error = std::string("pcm_prepare: ") + snd_strerror(rc); close_devices(); return false;
 }
 actual_sample_rate_ = capture_rate; actual_period_frames_ = capture_period;
 callback_ = callback; user_data_ = user_data;
 input_buffer_.assign(actual_period_frames_ * 2, 0.0F);
 output_buffer_.assign(actual_period_frames_ * 2, 0.0F);
 xruns_.store(0, std::memory_order_relaxed);
 recoveries_.store(0, std::memory_order_relaxed);
 short_transfers_.store(0, std::memory_order_relaxed);
 callbacks_.store(0, std::memory_order_relaxed);
 stop_requested_.store(false, std::memory_order_release); running_.store(true, std::memory_order_release);
 try { worker_ = std::thread(&AlsaAudioBackend::run, this); }
 catch (...) {
  running_.store(false, std::memory_order_release); close_devices();
  error = "audio_worker_thread_creation_failed"; return false;
 }
 error.clear(); return true;
}
void AlsaAudioBackend::stop() noexcept {
 stop_requested_.store(true, std::memory_order_release);
 // Drop wakes a blocking read/write so join does not depend on another device period.
 if (capture_) (void)snd_pcm_drop(capture_);
 if (playback_) (void)snd_pcm_drop(playback_);
 if (worker_.joinable()) worker_.join();
 running_.store(false, std::memory_order_release);
 callback_ = nullptr; user_data_ = nullptr;
 input_buffer_.clear(); output_buffer_.clear(); close_devices();
}
void AlsaAudioBackend::close_devices() noexcept {
 if (capture_) { snd_pcm_close(capture_); capture_ = nullptr; }
 if (playback_) { snd_pcm_close(playback_); playback_ = nullptr; }
}
void AlsaAudioBackend::run() noexcept {
 while (!stop_requested_.load(std::memory_order_acquire)) {
  snd_pcm_sframes_t got = snd_pcm_readi(capture_, input_buffer_.data(), static_cast<snd_pcm_uframes_t>(actual_period_frames_));
  if (got < 0) {
   if (got == -EPIPE) xruns_.fetch_add(1, std::memory_order_relaxed);
   recoveries_.fetch_add(1, std::memory_order_relaxed);
   const int recovered = snd_pcm_recover(capture_, static_cast<int>(got), 1);
   if (recovered < 0) break;
   std::fill(input_buffer_.begin(), input_buffer_.end(), 0.0F);
   continue;
  }
  if (static_cast<std::size_t>(got) < actual_period_frames_) {
   const auto start = static_cast<std::size_t>(got) * 2;
   std::fill(input_buffer_.begin() + static_cast<std::ptrdiff_t>(start), input_buffer_.end(), 0.0F);
   short_transfers_.fetch_add(1, std::memory_order_relaxed);
  }
  std::fill(output_buffer_.begin(), output_buffer_.end(), 0.0F);
  callback_(input_buffer_.data(), output_buffer_.data(), actual_period_frames_, user_data_);
  callbacks_.fetch_add(1, std::memory_order_relaxed);
  std::size_t written = 0;
  while (written < actual_period_frames_ && !stop_requested_.load(std::memory_order_acquire)) {
   const auto remaining = actual_period_frames_ - written;
   const auto offset = written * 2;
   const snd_pcm_sframes_t put = snd_pcm_writei(playback_, output_buffer_.data() + static_cast<std::ptrdiff_t>(offset),
                                                static_cast<snd_pcm_uframes_t>(remaining));
   if (put < 0) {
    if (put == -EPIPE) xruns_.fetch_add(1, std::memory_order_relaxed);
    recoveries_.fetch_add(1, std::memory_order_relaxed);
    const int recovered = snd_pcm_recover(playback_, static_cast<int>(put), 1);
    if (recovered < 0) { stop_requested_.store(true, std::memory_order_release); break; }
    continue;
   }
   if (put == 0) { short_transfers_.fetch_add(1, std::memory_order_relaxed); break; }
   written += static_cast<std::size_t>(put);
  }
 }
 running_.store(false, std::memory_order_release);
}
} // namespace hush
