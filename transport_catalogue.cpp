#include "transport_catalogue.h"

#include <unordered_set>

void TransportCatalogue::AddStop(std::string_view name, double latitude, double longitude) {
    Stop* new_stop_ptr = &stops_source.emplace_back(
        name,
        latitude, longitude
    );
    name_to_stop_[new_stop_ptr->name] = new_stop_ptr;
}

void TransportCatalogue::AddBus(std::string_view name, const std::vector<std::string_view>& raw_route, char route_type) {

    std::vector<Stop*> route;
    route.reserve(raw_route.size());

    for (const auto stop : raw_route) {
        route.push_back(name_to_stop_.at(stop));
    }

    Bus* new_bus_ptr = &buses_source.emplace_back(
            name,
            route,
            route_type
    );
    name_to_bus_[new_bus_ptr->name] = new_bus_ptr;

    //Добавляем автобус ко всем остановкам, через которые он проходит

    for (Stop* stop : new_bus_ptr->route) {
        stop_to_buses_[stop].insert(new_bus_ptr->name);
    }

}

void TransportCatalogue::PrintBusRoute(std::string_view bus_name) const {
    using namespace std::string_literals;
    if (name_to_bus_.count(bus_name) == 0) {
        std::cout << "Bus "s << bus_name << ": not found"s << std::endl;
        return;
    }

    std::cout << *name_to_bus_.at(bus_name) << std::endl;
}

void TransportCatalogue::PrintStopBuses(std::string_view stop_name) const {
    using namespace std::string_literals;
    if (name_to_stop_.count(stop_name) == 0) {
        std::cout << "Stop "s << stop_name << ": not found"s << std::endl;
        return;
    }

    Stop* stop = name_to_stop_.at(stop_name);
    if (stop_to_buses_.count(stop) == 0) {
        std::cout << "Stop "s << stop_name << ": no buses"s << std::endl;
        return;
    }

    const std::set<std::string_view>& buses = stop_to_buses_.at(stop);

    std::cout << "Stop "s << stop_name << ": buses "s;

    bool first = true;
    for (std::string_view bus : buses) {
        if (!first) {
            std::cout << ' ';
        }
        first = false;
        std::cout << bus;
    }
    std::cout << std::endl;
}

std::ostream& operator<<(std::ostream& o, const Bus& bus) {
    using namespace std::string_literals;

    o << "Bus "s << bus.name << ": "s;

    size_t total_stops = bus.route.size();
    size_t uniq_stops = std::unordered_set<Stop*>{bus.route.begin(), bus.route.end()}.size();
    double length = 0;

    for (size_t i = 1; i < bus.route.size(); ++i) {
        length += ComputeDistance(
                bus.route.at(i - 1)->coords,
                bus.route.at(i)->coords
        );
    }

    if (bus.type == REVERSIBLE) {
        total_stops = (total_stops * 2) - 1;
        length *= 2;
    }
    o << total_stops << " stops on route, "s << uniq_stops << " unique stops, "s
    << length << " route length"s;

    return o;
}
