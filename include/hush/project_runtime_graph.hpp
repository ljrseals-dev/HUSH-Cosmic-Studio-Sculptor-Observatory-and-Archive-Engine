#pragma once

#include "hush/project.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace hush {

// Control-thread selection of the persisted project bus graph used by the optional
// ALSA host. Projects without bus data retain the simple capture -> master fallback.
// This selects bus topology only; it does not invent a project track-to-bus mapping.
struct ProjectRuntimeGraph {
    BusGraph graph;
    std::string input_bus_id;
    std::string master_bus_id;
    std::size_t input_bus_index{0};
};

inline bool configure_project_runtime_graph(const Project& project,
                                             ProjectRuntimeGraph& out,
                                             RoutingError& error) {
    std::vector<Bus> buses;
    std::vector<BusRoute> routes;
    std::string input_bus_id = "capture";
    std::string master_bus_id = "master";

    if (!project.buses.empty()) {
        buses = project.buses;
        routes = project.routes;
        input_bus_id = buses.front().id;
        master_bus_id = buses.back().id;
        for (const auto& bus : buses) {
            if (bus.id == "capture") input_bus_id = bus.id;
            if (bus.id == "master") master_bus_id = bus.id;
        }
    } else {
        buses = {{"capture", "Capture"}, {"master", "Master"}};
        routes = {{"capture", "master", SendTap::PostFader, 1.0F}};
    }

    BusGraph candidate;
    if (!candidate.configure(std::move(buses), std::move(routes), error)) return false;

    std::size_t input_index = candidate.buses().size();
    for (std::size_t i = 0; i < candidate.buses().size(); ++i) {
        if (candidate.buses()[i].id == input_bus_id) {
            input_index = i;
            break;
        }
    }
    if (input_index == candidate.buses().size()) {
        error = {"project_input_bus_missing", input_bus_id,
                 "Selected input bus is absent from the configured project graph"};
        return false;
    }

    out.graph = std::move(candidate);
    out.input_bus_id = std::move(input_bus_id);
    out.master_bus_id = std::move(master_bus_id);
    out.input_bus_index = input_index;
    error = {};
    return true;
}

} // namespace hush
