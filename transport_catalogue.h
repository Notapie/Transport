#pragma once

#include <string>
#include <deque>
#include <vector>
#include <unordered_map>
#include <iostream>

#include "geo.h"

enum RouteType {
    ONE_SIDED,
    REVERSIBLE
};

struct Stop {
    Stop(std::string_view stop_name, double latitude, double longtitude)
    : name(std::string(stop_name)), coords({latitude, longtitude}) {}

    std::string name;
    Coordinates coords;
};

struct Bus {
    Bus(std::string_view bus_name, std::vector<Stop*>& bus_route, char route_type)
    : name(std::string(bus_name)), route(std::move(bus_route)), type(route_type == '-' ? REVERSIBLE : ONE_SIDED) {}

    std::string name;
    std::vector<Stop*> route;
    RouteType type = ONE_SIDED;
};

class TransportCatalogue {

public:
    void AddStop(std::string_view name, double latitude, double longitude);

    void AddBus(std::string_view name, const std::vector<std::string_view>& raw_route, char route_type);

    void PrintBus(std::string_view name) const;

private:
    std::deque<Stop> stops_source;
    std::deque<Bus> buses_source;
    std::unordered_map<std::string_view, Stop*> name_to_stop_;
    std::unordered_map<std::string_view, Bus*> name_to_bus_;

};

std::ostream& operator<<(std::ostream& o, const Bus& bus);
