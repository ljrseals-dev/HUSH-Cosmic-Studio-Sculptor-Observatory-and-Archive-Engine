#pragma once

#include "hush/project.hpp"
#include "hush/clip_renderer.hpp"
#include "hush/recording_session.hpp"
#include "hush/realtime_bus_engine_host.hpp"
#include "hush/realtime_track_bus_router.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <memory>
#include <string>
#include <vector>

namespace hush {

// Last completed block's routed bus-input levels. Values are linear amplitude,
// sampled before bus-graph processing; they are not post-insert/master-output meters.
struct BusInputMeterSnapshot {
    float peak_left{0.0F};
    float peak_right{0.0F};
    float rms_left{0.0F};
    float rms_right{0.0F};
};

// Project-aware capture -> track controls -> track bus -> master pipeline.
// Configure once on the control thread before starting audio. The process/callback
// path uses only preallocated storage and bounded queues; it performs no allocation,
// locking, logging, filesystem/network I/O, or GUI work.
class RealtimeProjectAudioPipeline {
public:
    bool configure(const Project& project, std::size_t max_frames, std::string& error);
    bool configure(const Project& project, std::vector<DecodedMediaBuffer> decoded_sources,
                   std::size_t max_frames, std::string& error);
    bool configure_from_project_directory(const Project& project,
                                          const std::filesystem::path& project_directory,
                                          std::size_t max_frames, std::string& error);
    void set_transport_playing(bool playing) noexcept {
        transport_playing_.store(playing, std::memory_order_release);
    }
    [[nodiscard]] bool transport_playing() const noexcept {
        return transport_playing_.load(std::memory_order_acquire);
    }
    void seek_transport(std::uint64_t frame) noexcept {
        transport_frame_.store(frame, std::memory_order_release);
    }
    [[nodiscard]] std::uint64_t transport_frame() const noexcept {
        return transport_frame_.load(std::memory_order_acquire);
    }
    [[nodiscard]] bool enqueue_track_control(std::size_t track_index, float gain,
                                             float pan, bool mute, bool solo) noexcept;
    [[nodiscard]] bool enqueue_bus_control(const std::string& bus_id, float gain,
                                           bool mute, bool solo) noexcept;
    // Control-thread-only safe graph replacement for insert/bus processing edits.
    // Builds the replacement off-callback and preserves the current graph on failure.
    [[nodiscard]] bool replace_project_bus_processing(const Project& project,
                                                      std::string& error);
    // Attach/detach on the control thread before/after the device runs. The capture
    // ring is SPSC and the disk writer remains on its own non-realtime worker.
    void set_recording_session(RecordingSession* session) noexcept { recording_ = session; }
    [[nodiscard]] bool process(const float* interleaved_input,
                               float* interleaved_output,
                               std::size_t frames) noexcept;
    static void alsa_callback(const float* input, float* output, std::size_t frames,
                              void* user_data) noexcept;

    [[nodiscard]] float input_peak() const noexcept { return input_peak_.load(std::memory_order_relaxed); }
    [[nodiscard]] float output_peak() const noexcept { return output_peak_.load(std::memory_order_relaxed); }
    [[nodiscard]] std::size_t bus_input_meter_count() const noexcept { return bus_meter_count_; }
    [[nodiscard]] BusInputMeterSnapshot bus_input_meter(std::size_t bus_index) const noexcept;
    [[nodiscard]] std::size_t bus_output_meter_count() const noexcept { return bus_ids_.size(); }
    [[nodiscard]] BusOutputMeterSnapshot bus_output_meter(std::size_t bus_index) const noexcept {
        return engine_host_.bus_output_meter(bus_index);
    }
    [[nodiscard]] const std::string& input_bus_id() const noexcept { return input_bus_id_; }
    [[nodiscard]] const std::string& master_bus_id() const noexcept { return master_bus_id_; }
    void reclaim_retired() noexcept { engine_host_.reclaim_retired(); }

private:
    struct RuntimeClip {
        std::size_t track_index{0};
        std::size_t source_index{0};
        std::int64_t timeline_start_frame{0};
        std::uint64_t source_start_frame{0};
        std::uint64_t frame_count{0};
        float gain{1.0F};
        std::uint64_t fade_in_frames{0};
        std::uint64_t fade_out_frames{0};
    };
    struct AtomicBusInputMeter {
        std::atomic<float> peak_left{0.0F};
        std::atomic<float> peak_right{0.0F};
        std::atomic<float> rms_left{0.0F};
        std::atomic<float> rms_right{0.0F};
    };

    RealtimeTrackBusRouter track_router_;
    RealtimeBusEngineHost engine_host_;
    std::vector<std::vector<float>> track_buffers_;
    std::vector<const float*> track_pointers_;
    std::vector<float> output_left_;
    std::vector<float> output_right_;
    std::vector<DecodedMediaBuffer> decoded_sources_;
    std::vector<RuntimeClip> runtime_clips_;
    std::unique_ptr<AtomicBusInputMeter[]> bus_input_meters_;
    std::size_t bus_meter_count_{0};
    RecordingSession* recording_{nullptr};
    std::vector<std::string> bus_ids_;
    std::string input_bus_id_;
    std::string master_bus_id_;
    std::size_t max_frames_{0};
    bool configured_{false};
    std::atomic<float> input_peak_{0.0F};
    std::atomic<float> output_peak_{0.0F};
    std::atomic<bool> transport_playing_{false};
    std::atomic<std::uint64_t> transport_frame_{0};
};

} // namespace hush
