#include "hush/realtime_bus_engine_host_audio_adapter.hpp"

#include <algorithm>
#include <cmath>

namespace hush {

bool RealtimeBusEngineHostAudioAdapter::configure(
    RealtimeBusEngineHost& host, std::size_t input_bus_index,
    std::size_t bus_count, std::size_t max_frames, std::string& error) {
    if (max_frames == 0 || bus_count == 0 || input_bus_index >= bus_count ||
        host.bus_count_for_control() != bus_count ||
        max_frames > host.max_frames_for_control()) {
        error = "host_adapter_topology_or_capacity_invalid";
        return false;
    }

    // Allocate all scratch before the device callback starts.
    std::vector<float> new_left(max_frames, 0.0F);
    std::vector<float> new_right(max_frames, 0.0F);
    std::vector<float> new_silence(max_frames, 0.0F);
    std::vector<BusAudioInputView> new_inputs(bus_count);

    left_.swap(new_left);
    right_.swap(new_right);
    silence_.swap(new_silence);
    bus_inputs_.swap(new_inputs);
    host_ = &host;
    input_bus_index_ = input_bus_index;
    max_frames_ = max_frames;
    error.clear();
    return true;
}

bool RealtimeBusEngineHostAudioAdapter::process(
    const float* interleaved_input, float* interleaved_output,
    std::size_t frames) noexcept {
    if (interleaved_output == nullptr) return false;
    if (frames > max_frames_ || host_ == nullptr ||
        (frames != 0 && interleaved_input == nullptr)) {
        std::fill_n(interleaved_output, frames * 2U, 0.0F);
        return false;
    }
    if (frames == 0) return true;

    for (std::size_t frame = 0; frame < frames; ++frame) {
        left_[frame] = interleaved_input[frame * 2U];
        right_[frame] = interleaved_input[frame * 2U + 1U];
    }
    for (auto& input : bus_inputs_) input = {silence_.data(), silence_.data()};
    bus_inputs_[input_bus_index_] = {left_.data(), right_.data()};

    const auto status = host_->process_to_stereo(
        bus_inputs_.data(), bus_inputs_.size(), frames,
        left_.data(), right_.data());
    if (status != RealtimeProcessStatus::Ok) {
        std::fill_n(interleaved_output, frames * 2U, 0.0F);
        return false;
    }

    for (std::size_t frame = 0; frame < frames; ++frame) {
        const float l = left_[frame];
        const float r = right_[frame];
        interleaved_output[frame * 2U] = std::isfinite(l) ? l : 0.0F;
        interleaved_output[frame * 2U + 1U] = std::isfinite(r) ? r : 0.0F;
    }
    return true;
}

void RealtimeBusEngineHostAudioAdapter::alsa_callback(
    const float* input, float* output, std::size_t frames, void* user_data) noexcept {
    auto* adapter = static_cast<RealtimeBusEngineHostAudioAdapter*>(user_data);
    if (adapter == nullptr) {
        if (output != nullptr) std::fill_n(output, frames * 2U, 0.0F);
        return;
    }
    (void)adapter->process(input, output, frames);
}

} // namespace hush
