#pragma once

#include "hush/realtime_bus_engine.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace hush {

// Bridges interleaved stereo device buffers to the planar RealtimeBusEngine.
// Configure on the control thread, after configuring the engine, before starting
// the device. One process() caller at a time; do not reconfigure while processing.
class RealtimeBusAudioAdapter {
public:
    bool configure(RealtimeBusEngine& engine, std::size_t input_bus_index,
                   std::size_t master_bus_index, std::size_t max_frames,
                   std::string& error);

    // Input/output are interleaved stereo float buffers. No allocation, locking,
    // logging, or I/O occurs here. On an invalid block, output is silenced.
    [[nodiscard]] bool process(const float* interleaved_input,
                               float* interleaved_output,
                               std::size_t frames) noexcept;

    // Signature-compatible with AlsaAudioBackend::ProcessCallback.
    static void alsa_callback(const float* input, float* output,
                              std::size_t frames, void* user_data) noexcept;

private:
    RealtimeBusEngine* engine_{nullptr};
    std::size_t input_bus_index_{0};
    std::size_t master_bus_index_{0};
    std::size_t max_frames_{0};
    std::vector<float> left_;
    std::vector<float> right_;
    std::vector<float> silence_;
    std::vector<BusAudioInputView> bus_inputs_;
};

} // namespace hush
