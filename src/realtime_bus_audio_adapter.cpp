#include "hush/realtime_bus_audio_adapter.hpp"

#include <algorithm>
#include <cmath>

namespace hush {

bool RealtimeBusAudioAdapter::configure(RealtimeBusEngine& engine,
                                        std::size_t input_bus_index,
                                        std::size_t master_bus_index,
                                        std::size_t max_frames,
                                        std::string& error) {
    if (max_frames == 0 || max_frames > engine.max_frames()) {
        error = "adapter_frame_capacity_invalid";
        return false;
    }
    if (engine.bus_count() == 0 || input_bus_index >= engine.bus_count() ||
        master_bus_index >= engine.bus_count()) {
        error = "adapter_bus_index_out_of_range";
        return false;
    }

    // All allocations happen before the device starts.
    std::vector<float> new_left(max_frames, 0.0F);
    std::vector<float> new_right(max_frames, 0.0F);
    std::vector<float> new_silence(max_frames, 0.0F);
    std::vector<BusAudioInputView> new_inputs(engine.bus_count());

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

bool RealtimeBusAudioAdapter::process(const float* interleaved_input,
                                      float* interleaved_output,
                                      std::size_t frames) noexcept {
    if (interleaved_output == nullptr) return false;
    if (frames > max_frames_ || engine_ == nullptr ||
        (frames != 0 && interleaved_input == nullptr)) {
        if (frames <= max_frames_) {
            std::fill_n(interleaved_output, frames * 2, 0.0F);
        }
        return false;
    }
    if (frames == 0) return true;

    for (std::size_t frame = 0; frame < frames; ++frame) {
        left_[frame] = interleaved_input[frame * 2];
        right_[frame] = interleaved_input[frame * 2 + 1];
    }
    for (auto& input : bus_inputs_) {
        input = {silence_.data(), silence_.data()};
    }
    bus_inputs_[input_bus_index_] = {left_.data(), right_.data()};

    const auto status = engine_->process(bus_inputs_.data(), bus_inputs_.size(), frames);
    if (status != RealtimeProcessStatus::Ok) {
        std::fill_n(interleaved_output, frames * 2, 0.0F);
        return false;
    }

    const auto master = engine_->post_fader(master_bus_index_, frames);
    if (master.left == nullptr || master.right == nullptr) {
        std::fill_n(interleaved_output, frames * 2, 0.0F);
        return false;
    }
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const float l = master.left[frame];
        const float r = master.right[frame];
        interleaved_output[frame * 2] = std::isfinite(l) ? l : 0.0F;
        interleaved_output[frame * 2 + 1] = std::isfinite(r) ? r : 0.0F;
    }
    return true;
}

void RealtimeBusAudioAdapter::alsa_callback(const float* input, float* output,
                                            std::size_t frames, void* user_data) noexcept {
    auto* adapter = static_cast<RealtimeBusAudioAdapter*>(user_data);
    if (adapter == nullptr) {
        if (output != nullptr) std::fill_n(output, frames * 2, 0.0F);
        return;
    }
    (void)adapter->process(input, output, frames);
}

} // namespace hush
