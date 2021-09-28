#pragma once

#include <string>
#include <deque>
#include <vector>
#include <unordered_map>
#include <set>
#include <utility>

#include "geo/geo.h"
#include "domain.h"

namespace transport_catalogue {
    using namespace domain;

    namespace detail {

        struct StopsHasher {
            size_t operator()(const std::pair<Stop*, Stop*>& stops) const;
        private:
            std::hash<const void*> ptr_hasher_;
        };

    } //namespace detail

    class TransportCatalogue {
    public:
        void AddStop(std::string_view name, double latitude, double longitude);
        void AddDistance(std::string_view stop_name, std::string_view destination_name, int distace);

        void AddBus(std::string_view name, const std::vector<std::string_view>& raw_route, char route_type);

        bool IsBusExists(std::string_view bus_name) const noexcept;
        bool IsStopExists(std::string_view stop_name) const noexcept;

        RouteInfo GetRouteInfo(std::string_view bus_name) const;
        std::vector<std::string> GetStopBuses(std::string_view stop_name) const;

        const std::deque<Bus>& GetBuses() const;

    private:
        std::deque<Stop> stops_source_;
        std::deque<Bus> buses_source_;
        std::unordered_map<std::string_view, Stop*> name_to_stop_;
        std::unordered_map<std::string_view, Bus*> name_to_bus_;
        std::unordered_map<Stop*, std::set<std::string_view>> stop_to_buses_;
        std::unordered_map<std::pair<Stop*, Stop*>, int, detail::StopsHasher> stops_to_length_;

        double GetRouteGeoDistance(const Bus& bus) const;
        int GetRouteRealDistance(const Bus& bus) const;

        int GetRealLength(Stop* first_stop, Stop* second_stop) const;

    };

} //namespace transport_catalogue
