#pragma once

#include <string>
#include <vector>

#include "geo/geo.h"

namespace transport_catalogue::domain {

    enum class RouteType {
        ONE_SIDED,
        REVERSIBLE
    };

    struct Stop {
        Stop(std::string_view stop_name, double latitude, double longtitude);

        std::string name;
        geo::Coordinates coords;
    };

    struct Bus {
        Bus(std::string_view bus_name, std::vector<Stop*>& bus_route, char route_type);

        std::string name;
        std::vector<Stop*> route;
        RouteType type = RouteType::ONE_SIDED;
    };

    struct RouteInfo {
        size_t total_stops = 0;
        size_t uniq_stops = 0;
        double real_length = 0.0;
        double curvature = 0.0;
    };

} // namespace transport_catalogue::domain
