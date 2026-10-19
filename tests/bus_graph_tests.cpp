#include "hush/bus_graph.hpp"
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
 assert(!graph.configure(buses,{{"master","master",SendTap::PostFader,1.0F}},error) && error.code=="bus_route_self_cycle");
 return 0;
}
