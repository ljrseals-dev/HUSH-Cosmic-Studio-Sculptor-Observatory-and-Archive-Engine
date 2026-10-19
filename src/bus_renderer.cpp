#include "hush/bus_renderer.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <utility>

namespace hush {

bool render_bus_graph(const BusGraph& graph,
                      const std::vector<BusAudioInput>& inputs,
                      std::vector<RenderedBus>& outputs,
                      BusRenderError& error) {
    outputs.clear();
    if (!graph.valid()) {
        error = {"bus_graph_invalid", "", "Bus graph must be configured and valid"};
        return false;
    }
    if (inputs.size() != graph.buses().size()) {
        error = {"bus_input_count", "", "Exactly one audio input is required for each bus"};
        return false;
    }

    std::unordered_map<std::string, const BusAudioInput*> input_by_id;
    input_by_id.reserve(inputs.size());
    std::size_t frames = 0;
    bool first = true;
    for (const auto& input : inputs) {
        if (!std::isfinite(input.fader_gain) || input.fader_gain < 0.0F || input.fader_gain > 16.0F) {
            error = {"bus_fader_gain_invalid", input.bus_id, "Fader gain must be finite and in [0, 16]"};
            return false;
        }
        if (input.audio.left.size() != input.audio.right.size()) {
            error = {"bus_channel_length_mismatch", input.bus_id, "Left and right channels must have equal lengths"};
            return false;
        }
        if (first) {
            frames = input.audio.left.size();
            first = false;
        } else if (input.audio.left.size() != frames) {
            error = {"bus_frame_count_mismatch", input.bus_id, "All bus inputs must have the same frame count"};
            return false;
        }
        if (!input_by_id.emplace(input.bus_id, &input).second) {
            error = {"bus_input_duplicate", input.bus_id, "Bus audio input IDs must be unique"};
            return false;
        }
    }
    for (const auto& bus : graph.buses()) {
        if (input_by_id.find(bus.id) == input_by_id.end()) {
            error = {"bus_input_missing", bus.id, "No audio input was supplied for this bus"};
            return false;
        }
    }

    const auto order = graph.render_order();
    outputs.reserve(order.size());
    std::unordered_map<std::string, std::size_t> output_index;
    output_index.reserve(order.size());
    for (const auto& id : order) {
        const auto* input = input_by_id.at(id);
        RenderedBus bus;
        bus.bus_id = id;
        bus.pre_fader = input->audio;
        bus.post_fader.left.assign(frames, 0.0F);
        bus.post_fader.right.assign(frames, 0.0F);
        output_index.emplace(id, outputs.size());
        outputs.push_back(std::move(bus));
    }

    // A topological order ensures every upstream send has reached a bus before its
    // fader is applied and that bus sends onward.
    for (const auto& id : order) {
        auto& source = outputs[output_index.at(id)];
        const auto& source_input = *input_by_id.at(id);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            source.post_fader.left[frame] = source.pre_fader.left[frame] * source_input.fader_gain;
            source.post_fader.right[frame] = source.pre_fader.right[frame] * source_input.fader_gain;
        }
        for (const auto& route : graph.routes()) {
            if (route.source_bus_id != id) continue;
            const auto& tap = route.tap == SendTap::PreFader ? source.pre_fader : source.post_fader;
            auto& destination = outputs[output_index.at(route.destination_bus_id)].pre_fader;
            for (std::size_t frame = 0; frame < frames; ++frame) {
                destination.left[frame] += tap.left[frame] * route.gain;
                destination.right[frame] += tap.right[frame] * route.gain;
            }
        }
    }
    error = {};
    return true;
}

} // namespace hush
