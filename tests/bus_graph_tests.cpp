#include "hush/bus_graph.hpp"
#include "hush/project_runtime_graph.hpp"
#include "hush/project.hpp"
#include <algorithm>
#include <cassert>
int main() {
 using namespace hush; RoutingError error; BusGraph graph;
 const std::vector<Bus> buses{{"drums","Drums"},{"music","Music"},{"master","Master"}};
 assert(graph.configure(buses,{{"drums","music",SendTap::PostFader,0.5F},{"music","master",SendTap::PreFader,1.0F}},error));
 const auto order=graph.render_order(); assert(order.size()==3);
 assert(std::find(order.begin(),order.end(),"drums")<std::find(order.begin(),order.end(),"music"));
 assert(std::find(order.begin(),order.end(),"music")<std::find(order.begin(),order.end(),"master"));
 assert(!graph.configure(buses,{{"drums","music",SendTap::PostFader,1.0F},{"music","drums",SendTap::PostFader,1.0F}},error));
 assert(error.code=="bus_route_cycle" && !graph.valid() && graph.render_order().empty());
 assert(!graph.configure(buses,{{"drums","missing",SendTap::PostFader,1.0F}},error) && error.code=="bus_route_unknown_bus");
 assert(!graph.configure(buses,{{"drums","master",SendTap::PostFader,-1.0F}},error) && error.code=="bus_route_gain_invalid");
 assert(!graph.configure({{"dup","One"},{"dup","Two"}},{},error) && error.code=="bus_id_duplicate");
 assert(!graph.configure({{"gain","Invalid",17.0F,false,false}},{},error) && error.code=="bus_gain_invalid");
 assert(!graph.configure(buses,{{"master","master",SendTap::PostFader,1.0F}},error) && error.code=="bus_route_self_cycle");

 // The host uses the persisted bus graph and deterministic input/master IDs.
 Project project;
 project.buses = {{"capture","Capture"},{"music","Music"},{"master","Master"}};
 project.routes = {{"capture","music",SendTap::PostFader,0.75F},
                   {"music","master",SendTap::PostFader,1.0F}};
 ProjectRuntimeGraph runtime;
 assert(configure_project_runtime_graph(project,runtime,error));
 assert(runtime.graph.valid());
 assert(runtime.input_bus_id=="capture" && runtime.input_bus_index==0);
 assert(runtime.master_bus_id=="master" && runtime.graph.buses().size()==3);

 // Older projects with no bus arrays retain the capture -> master fallback.
 Project legacy;
 assert(configure_project_runtime_graph(legacy,runtime,error));
 assert(runtime.input_bus_id=="capture" && runtime.input_bus_index==0);
 assert(runtime.master_bus_id=="master" && runtime.graph.buses().size()==2);
 assert(runtime.graph.routes().size()==1);

 // Invalid persisted routing fails closed instead of silently replacing it.
 project.routes = {{"capture","music",SendTap::PostFader,1.0F},
                   {"music","capture",SendTap::PostFader,1.0F}};
 assert(!configure_project_runtime_graph(project,runtime,error));
 assert(error.code=="bus_route_cycle");
 return 0;
}
