#include "hush/published_realtime_bus_audio_adapter.hpp"

#include <algorithm>
#include <cmath>

namespace hush {

bool PublishedRealtimeBusAudioAdapter::configure(
    PublishedRealtimeBusEngine& engine, std::size_t bus_count,
    std::size_t input_bus_index, std::size_t master_bus_index,
    std::size_t max_frames, std::string& error) {
    if (bus_count == 0 || max_frames == 0 ||
        input_bus_index >= bus_count || master_bus_index >= bus_count) {
        error = "adapter_configuration_out_of_range";
        return false;
    }

    // Allocate all callback scratch before device start.
    std::vector<float> new_left(max_frames, 0.0F);
    std::vector<float> new_right(max_frames, 0.0F);
    std::vector<float> new_silence(max_frames, 0.0F);
    std::vector<BusAudioInputView> new_inputs(bus_count);
    left_.swap(new_left);
    right_.swap(new_right);
    silence_.swap(new_silence);
    bus_inputs_.swap(new_inputs);
    engine_ = &engine;
    input_bus_index_ = input_bus_index;
    master_bus_index_ = master_bus_index;
    max_frames_ = max_frames;
    error.clear();
    return true;
}

bool PublishedRealtimeBusAudioAdapter::process(
    const float* interleaved_input, float* interleaved_output,
    std::size_t frames) noexcept {
    if (interleaved_output == nullptr) return false;
    if (frames > max_frames_ || engine_ == nullptr ||
        (frames != 0 && interleaved_input == nullptr)) {
        if (frames <= max_frames_) std::fill_n(interleaved_output, frames * 2, 0.0F);
        return false;
    }
    if (frames == 0) return true;

    for (std::size_t frame = 0; frame < frames; ++frame) {
        left_[frame] = interleaved_input[frame * 2];
        right_[frame] = interleaved_input[frame * 2 + 1];
    }
    for (auto& input : bus_inputs_)
        input = {silence_.data(), silence_.data()};
    bus_inputs_[input_bus_index_] = {left_.data(), right_.data()};

    std::vector<float> unused; // Never used; callback path remains preallocated.
    (void)unused;
    // The guarded engine copies master samples before releasing the graph lifetime guard.
    // Reuse left_/right_ as planar output only after processing has consumed them.
    const auto status = engine_->process_to_master(
        bus_inputs_.data(), bus_inputs_.size(), frames, master_bus_index_,
        left_.data(), right_.data());
    if (status != RealtimeProcessStatus::Ok) {
        std::fill_n(interleaved_output, frames * 2, 0.0F);
        return false;
    }
    for (std::size_t frame = 0; frame < frames; ++frame) {
        interleaved_output[frame * 2] = std::isfinite(left_[frame]) ? left_[frame] : 0.0F;
        interleaved_output[frame * 2 + 1] = std::isfinite(right_[frame]) ? right_[frame] : 0.0F;
    }
    return true;
}

void PublishedRealtimeBusAudioAdapter::alsa_callback(
    const float* input, float* output, std::size_t frames, void* user_data) noexcept {
    auto* adapter = static_cast<PublishedRealtimeBusAudioAdapter*>(user_data);
    if (adapter == nullptr) {
        if (output != nullptr) std::fill_n(output, frames * 2, 0.0F);
        return;
    }
    (void)adapter->process(input, output, frames);
}

} // namespace hush
