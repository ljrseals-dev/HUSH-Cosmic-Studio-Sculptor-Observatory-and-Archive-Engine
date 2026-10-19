#pragma once

#include "hush/realtime_bus_engine_host.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace hush {

// Preallocated interleaved-stereo bridge for RealtimeBusEngineHost.
// Configure before starting the device. Input bus index and bus count must remain
// stable while active; stop the audio reader before changing either. Same-topology
// engine replacements may be published while process() is running.
class RealtimeBusEngineHostAudioAdapter {
public:
    bool configure(RealtimeBusEngineHost& host, std::size_t input_bus_index,
                   std::size_t bus_count, std::size_t max_frames, std::string& error);
    [[nodiscard]] bool process(const float* interleaved_input, float* interleaved_output,
                               std::size_t frames) noexcept;
    static void alsa_callback(const float* input, float* output, std::size_t frames,
                              void* user_data) noexcept;
private:
    RealtimeBusEngineHost* host_{nullptr};
    std::size_t input_bus_index_{0};
    std::size_t max_frames_{0};
    std::vector<float> left_, right_, silence_;
    std::vector<BusAudioInputView> bus_inputs_;
};

} // namespace hush
