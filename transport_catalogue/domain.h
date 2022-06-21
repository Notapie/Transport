#pragma once

#include <string>
#include <vector>

#include "geo/geo.h"

namespace transport_catalogue::domain {

    enum class RouteType {
        ROUND_TRIP = 0,
        ONE_WAY = 1
    };

    struct Stop {
        Stop(std::string_view stop_name, double latitude, double longtitude);

        std::string name;
        geo::Coordinates coords;
    };

    struct Bus {
        Bus(std::string_view bus_name, std::vector<Stop*>& bus_route, RouteType type);

        std::string name;
        std::vector<Stop*> route;
        RouteType type = RouteType::ROUND_TRIP;
    };

    struct RouteInfo {
        size_t total_stops = 0;
        size_t uniq_stops = 0;
        int real_length = 0;
        double curvature = 0.0;
    };

} // namespace transport_catalogue::domain
