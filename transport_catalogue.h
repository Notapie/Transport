#pragma once

#include <string>
#include <deque>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <set>
#include <utility>

#include "geo.h"

namespace transport_catalogue {

    namespace detail {

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

        struct StopsHasher {
            size_t operator()(const std::pair<Stop*, Stop*>& stops) const;
        private:
            std::hash<const void*> ptr_hasher_;
        };

        struct Bus {
            Bus(std::string_view bus_name, std::vector<Stop*>& bus_route, char route_type)
                    : name(std::string(bus_name)), route(std::move(bus_route)), type(route_type == '-' ? REVERSIBLE : ONE_SIDED) {}

            std::string name;
            std::vector<Stop*> route;
            RouteType type = ONE_SIDED;
        };

    }


    class TransportCatalogue {

    public:
        void AddStop(std::string_view name, double latitude, double longitude);
        void AddDistance(std::string_view stop_name, std::string_view destination_name, int distace);

        void AddBus(std::string_view name, const std::vector<std::string_view>& raw_route, char route_type);

        void PrintBusRoute(std::string_view bus_name) const;
        void PrintStopBuses(std::string_view stop_name) const;


    private:
        std::deque<detail::Stop> stops_source;
        std::deque<detail::Bus> buses_source;
        std::unordered_map<std::string_view, detail::Stop*> name_to_stop_;
        std::unordered_map<std::string_view, detail::Bus*> name_to_bus_;
        std::unordered_map<detail::Stop*, std::set<std::string_view>> stop_to_buses_;
        std::unordered_map<std::pair<detail::Stop*, detail::Stop*>, int, detail::StopsHasher> stops_to_length_;

        double GetRouteGeoDistance(const detail::Bus& bus) const;
        int GetRouteRealDistance(const detail::Bus& bus) const;

        int GetRealLength(detail::Stop* first_stop, detail::Stop* second_stop) const;

    };

}
