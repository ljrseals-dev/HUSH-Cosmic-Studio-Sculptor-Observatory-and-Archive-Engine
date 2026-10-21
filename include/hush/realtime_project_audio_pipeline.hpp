#pragma once

#include "hush/project.hpp"
#include "hush/realtime_bus_engine_host.hpp"
#include "hush/realtime_track_bus_router.hpp"

#include <atomic>
#include <cstddef>
#include <string>
#include <vector>

namespace hush {

// Project-aware capture -> track controls -> track bus -> master pipeline.
// Configure once on the control thread before starting audio. The process/callback
// path uses only preallocated storage and bounded queues; it performs no allocation,
// locking, logging, filesystem/network I/O, or GUI work.
class RealtimeProjectAudioPipeline {
public:
    bool configure(const Project& project, std::size_t max_frames, std::string& error);
    [[nodiscard]] bool enqueue_track_control(std::size_t track_index, float gain,
                                             float pan, bool mute, bool solo) noexcept;
    [[nodiscard]] bool process(const float* interleaved_input,
                               float* interleaved_output,
                               std::size_t frames) noexcept;
    static void alsa_callback(const float* input, float* output, std::size_t frames,
                              void* user_data) noexcept;

    [[nodiscard]] std::uint64_t xrun_count_placeholder() const noexcept { return 0U; }
    [[nodiscard]] float input_peak() const noexcept { return input_peak_.load(std::memory_order_relaxed); }
    [[nodiscard]] float output_peak() const noexcept { return output_peak_.load(std::memory_order_relaxed); }
    [[nodiscard]] const std::string& input_bus_id() const noexcept { return input_bus_id_; }
    [[nodiscard]] const std::string& master_bus_id() const noexcept { return master_bus_id_; }
    void reclaim_retired() noexcept { engine_host_.reclaim_retired(); }

private:
    RealtimeTrackBusRouter track_router_;
    RealtimeBusEngineHost engine_host_;
    std::vector<std::vector<float>> track_buffers_;
    std::vector<const float*> track_pointers_;
    std::vector<float> output_left_;
    std::vector<float> output_right_;
    std::string input_bus_id_;
    std::string master_bus_id_;
    std::size_t max_frames_{0};
    bool configured_{false};
    std::atomic<float> input_peak_{0.0F};
    std::atomic<float> output_peak_{0.0F};
};

} // namespace hush
