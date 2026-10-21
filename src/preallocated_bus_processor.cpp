#include "hush/preallocated_bus_processor.hpp"

#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace hush {

bool PreallocatedBusProcessor::configure(const BusGraph& graph, std::size_t max_frames,
                                         ProcessorSetupError& error,
                                         const std::vector<BusInsert>& inserts) {
    configured_ = false;
    buses_.clear();
    routes_.clear();
    render_order_.clear();
    max_frames_ = 0;

    if (!graph.valid()) {
        error = {"bus_graph_invalid", "", "Bus graph must be configured and valid"};
        return false;
    }
    if (max_frames == 0) {
        error = {"frame_capacity_invalid", "", "Maximum frame capacity must be greater than zero"};
        return false;
    }

    std::unordered_map<std::string, std::size_t> indices;
    indices.reserve(graph.buses().size());
    buses_.reserve(graph.buses().size());
    for (std::size_t i = 0; i < graph.buses().size(); ++i) {
        const auto& bus = graph.buses()[i];
        indices.emplace(bus.id, i);
        RuntimeBus runtime;
        runtime.id = bus.id;
        runtime.pre_left.assign(max_frames, 0.0F);
        runtime.pre_right.assign(max_frames, 0.0F);
        runtime.post_left.assign(max_frames, 0.0F);
        runtime.post_right.assign(max_frames, 0.0F);
        buses_.push_back(std::move(runtime));
    }

    std::unordered_set<std::string> insert_ids;
    insert_ids.reserve(inserts.size());
    for (const auto& insert : inserts) {
        const auto bus = indices.find(insert.bus_id);
        if (insert.id.empty() || !insert_ids.insert(insert.id).second ||
            bus == indices.end() || !std::isfinite(insert.gain) ||
            insert.gain < 0.0F || insert.gain > 16.0F) {
            error = {"bus_insert_invalid", insert.id,
                     "Insert ID, bus reference, and gain must be valid"};
            buses_.clear();
            routes_.clear();
            render_order_.clear();
            return false;
        }
        buses_[bus->second].inserts.push_back(
            {insert.kind, insert.enabled, insert.gain});
    }

    routes_.reserve(graph.routes().size());
    for (const auto& route : graph.routes()) {
        const auto source = indices.find(route.source_bus_id);
        const auto destination = indices.find(route.destination_bus_id);
        if (source == indices.end() || destination == indices.end()) {
            error = {"bus_route_unknown_bus", route.source_bus_id,
                     "Validated graph unexpectedly references an unknown bus"};
            buses_.clear();
            routes_.clear();
            return false;
        }
        routes_.push_back({source->second, destination->second, route.tap, route.gain});
    }

    const auto order = graph.render_order();
    render_order_.reserve(order.size());
    for (const auto& id : order) {
        const auto found = indices.find(id);
        if (found == indices.end()) {
            error = {"bus_render_order_invalid", id, "Render order references an unknown bus"};
            buses_.clear();
            routes_.clear();
            render_order_.clear();
            return false;
        }
        render_order_.push_back(found->second);
    }

    max_frames_ = max_frames;
    configured_ = true;
    error = {};
    return true;
}

RealtimeProcessStatus PreallocatedBusProcessor::process(const BusInputView* inputs,
                                                        std::size_t input_count,
                                                        std::size_t frame_count) noexcept {
    if (!configured_) return RealtimeProcessStatus::NotConfigured;
    if (input_count != buses_.size() || (input_count != 0 && inputs == nullptr))
        return RealtimeProcessStatus::InvalidInputCount;
    if (frame_count > max_frames_) return RealtimeProcessStatus::FrameCountExceedsCapacity;

    for (std::size_t i = 0; i < input_count; ++i) {
        if (inputs[i].left == nullptr || inputs[i].right == nullptr)
            return RealtimeProcessStatus::NullChannel;
        if (!std::isfinite(inputs[i].fader_gain) || inputs[i].fader_gain < 0.0F ||
            inputs[i].fader_gain > 16.0F)
            return RealtimeProcessStatus::InvalidFaderGain;
    }

    for (std::size_t i = 0; i < buses_.size(); ++i) {
        auto& bus = buses_[i];
        for (std::size_t frame = 0; frame < frame_count; ++frame) {
            bus.pre_left[frame] = inputs[i].left[frame];
            bus.pre_right[frame] = inputs[i].right[frame];
            bus.post_left[frame] = 0.0F;
            bus.post_right[frame] = 0.0F;
        }
    }

    for (const auto source_index : render_order_) {
        auto& source = buses_[source_index];
        // Built-in inserts execute in persisted order before the bus fader. All
        // storage was allocated by configure(); this loop never resizes buffers.
        for (const auto& insert : source.inserts) {
            if (!insert.enabled) continue;
            for (std::size_t frame = 0; frame < frame_count; ++frame) {
                if (insert.kind == InsertKind::Gain) {
                    source.pre_left[frame] *= insert.gain;
                    source.pre_right[frame] *= insert.gain;
                } else {
                    source.pre_left[frame] = std::tanh(source.pre_left[frame]);
                    source.pre_right[frame] = std::tanh(source.pre_right[frame]);
                }
            }
        }
        const float fader_gain = inputs[source_index].fader_gain;
        for (std::size_t frame = 0; frame < frame_count; ++frame) {
            source.post_left[frame] = source.pre_left[frame] * fader_gain;
            source.post_right[frame] = source.pre_right[frame] * fader_gain;
        }

        for (const auto& route : routes_) {
            if (route.source != source_index) continue;
            const auto& tap_left = route.tap == SendTap::PreFader
                ? source.pre_left : source.post_left;
            const auto& tap_right = route.tap == SendTap::PreFader
                ? source.pre_right : source.post_right;
            auto& destination = buses_[route.destination];
            for (std::size_t frame = 0; frame < frame_count; ++frame) {
                destination.pre_left[frame] += tap_left[frame] * route.gain;
                destination.pre_right[frame] += tap_right[frame] * route.gain;
            }
        }
    }
    return RealtimeProcessStatus::Ok;
}

const std::string& PreallocatedBusProcessor::bus_id(std::size_t index) const {
    return buses_.at(index).id;
}

StereoConstView PreallocatedBusProcessor::pre_fader(std::size_t bus_index,
                                                    std::size_t frames) const noexcept {
    if (!configured_ || bus_index >= buses_.size() || frames > max_frames_) return {};
    const auto& bus = buses_[bus_index];
    return {bus.pre_left.data(), bus.pre_right.data(), frames};
}

StereoConstView PreallocatedBusProcessor::post_fader(std::size_t bus_index,
                                                     std::size_t frames) const noexcept {
    if (!configured_ || bus_index >= buses_.size() || frames > max_frames_) return {};
    const auto& bus = buses_[bus_index];
    return {bus.post_left.data(), bus.post_right.data(), frames};
}

} // namespace hush
