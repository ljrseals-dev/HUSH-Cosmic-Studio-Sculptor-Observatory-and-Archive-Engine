#pragma once

#include "hush/published_realtime_bus_engine.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace hush {

// Device adapter for PublishedRealtimeBusEngine. All buffers are allocated in
// configure() before starting the device; process() is allocation/lock/I/O-free.
class PublishedRealtimeBusAudioAdapter {
public:
    bool configure(PublishedRealtimeBusEngine& engine, std::size_t bus_count,
                   std::size_t input_bus_index, std::size_t master_bus_index,
                   std::size_t max_frames, std::string& error);

    [[nodiscard]] bool process(const float* interleaved_input,
                               float* interleaved_output,
                               std::size_t frames) noexcept;

    static void alsa_callback(const float* input, float* output,
                              std::size_t frames, void* user_data) noexcept;

private:
    PublishedRealtimeBusEngine* engine_{nullptr};
    std::size_t input_bus_index_{0};
    std::size_t master_bus_index_{0};
    std::size_t max_frames_{0};
    std::vector<float> left_;
    std::vector<float> right_;
    std::vector<float> silence_;
    std::vector<BusAudioInputView> bus_inputs_;
};

} // namespace hush
