#include "domain.h"

namespace transport_catalogue::domain {

    Stop::Stop(std::string_view stop_name, double latitude, double longtitude)
    : name(std::string(stop_name)), coords({latitude, longtitude}) {}

    Bus::Bus(std::string_view bus_name, std::vector<Stop*>& bus_route, RouteType type)
    : name(std::string(bus_name)), route(std::move(bus_route)), type(type) {}

} // namespace transport_catalogue::domain
