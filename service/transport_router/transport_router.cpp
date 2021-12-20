#include "transport_router.h"

#include <unordered_set>

using namespace std::literals;

namespace transport_catalogue::service {

    template<typename It>
    size_t CountVertexes(It begin, It end) {
        std::unordered_set<const domain::Stop*> uniq_stops;
        size_t count = 0;
        for (It it = begin; it != end; ++it) {
            for (const domain::Stop* stop : it->route) {
                if (uniq_stops.count(stop) == 0) {
                    uniq_stops.insert(stop);
                    ++count;
                }
                ++count;
            }
            count += it->type == RouteType::ONE_WAY ? it->route.size() : 1;
        }
        return count;
    }

    TransportRouter::TransportRouter(const TransportCatalogue& catalogue) : catalogue_(catalogue) {}

    TransportRouter::TransportRouter(RouterSettings settings, const TransportCatalogue& catalogue)
    : bus_wait_time_(settings.bus_wait_time), bus_velocity_(settings.bus_velocity), catalogue_(catalogue) {}

    void TransportRouter::UpdateSettings(RouterSettings settings) {
        bus_wait_time_ = settings.bus_wait_time;
        bus_velocity_ = settings.bus_velocity;
    }

    void TransportRouter::BuildGraph() {
        const std::deque<domain::Bus>& buses = catalogue_.GetBuses();
        graph_ = graph::DirectedWeightedGraph<double>(CountVertexes(buses.begin(), buses.end()));
        for (const domain::Bus& bus : buses) {
            AddBusRoute(bus);
        }

        router_ptr_ = std::make_unique<graph::Router<double>>(graph_);
    }

    void TransportRouter::AddBusRoute(const domain::Bus& bus) {
        if (bus_velocity_ <= 0) {
            throw std::logic_error("invalid bus velocity: \""s + std::to_string(bus_velocity_) + "\""s);
        }
        size_t stops_count = bus.route.size();
        if (stops_count == 0) {
            return;
        }

        const Stop* prev_stop = bus.route.at(0);
        graph::VertexId prev_stop_id = CreateVertex(prev_stop);
        for (size_t i = 1; i < stops_count; ++i) {
            const Stop* current_stop = bus.route.at(i);
            graph::VertexId current_stop_id = CreateVertex(current_stop);

            double distance = catalogue_.GetRealLength(prev_stop, current_stop);
            double time = distance / (bus_velocity_ / 0.06);

            edge_to_route_[graph_.AddEdge({prev_stop_id, current_stop_id, time})] = &bus;

            prev_stop = current_stop;
            prev_stop_id = current_stop_id;
        }

        //Убрать дублирование
        prev_stop_id = CreateVertex(prev_stop);
        if (bus.type == domain::RouteType::ONE_WAY) {
            for (int i = int(stops_count) - 2; i >= 0; --i) {
                const Stop* current_stop = bus.route.at(i);
                graph::VertexId current_stop_id = CreateVertex(current_stop);

                double distance = catalogue_.GetRealLength(prev_stop, current_stop);
                double time = distance / (bus_velocity_ / 0.06);

                edge_to_route_[graph_.AddEdge({prev_stop_id, current_stop_id, time})] = &bus;

                prev_stop = current_stop;
                prev_stop_id = current_stop_id;
            }
        }
    }

    graph::VertexId TransportRouter::CreateVertex(const domain::Stop* stop) {
        if (!stop_to_hub_.count(stop)) {
            stop_to_hub_[stop] = vertex_counter_++;
        }
        graph::VertexId hub_id = stop_to_hub_.at(stop);
        graph::VertexId stop_id = vertex_counter_++;

        // Путь на остановку и с остановки не принадлежит никакому маршруту, поэтому присваиваем nullptr
        graph::EdgeId from_hub = graph_.AddEdge({hub_id, stop_id, static_cast<double>(bus_wait_time_)});
        edge_to_route_[graph_.AddEdge({stop_id, hub_id, 0})] = nullptr;
        edge_to_route_[from_hub] = nullptr;

        waiting_edge_to_stop_[from_hub] = stop;

        return stop_id;
    }

    std::optional<TransportRouter::Route> TransportRouter::GetRoute(std::string_view from, std::string_view to) const {
        const Stop* from_ptr = catalogue_.GetStop(from);
        const Stop* to_ptr = catalogue_.GetStop(to);

        if (from_ptr == nullptr || to_ptr == nullptr || stop_to_hub_.count(from_ptr) == 0 || stop_to_hub_.count(to_ptr) == 0) {
            return std::nullopt;
        }

        graph::VertexId from_vert = stop_to_hub_.at(from_ptr);
        graph::VertexId to_vert = stop_to_hub_.at(to_ptr);

        std::optional<graph::Router<double>::RouteInfo> route = router_ptr_->BuildRoute(from_vert, to_vert);

        //Если маршрут построить не удалось, то возвращаем пустой optional
        if (!route) {
            return std::nullopt;
        }

        Route result;
        result.total_time = route->weight;

        IntervalType interval_type = IntervalType::WAITING;
        std::string interval_route_name;
        std::string waiting_stop_name;
        double interval_time = 0.0;
        size_t interval_stops_count = 1;
        for (graph::EdgeId edge_id : route->edges) {

            const Bus* current_route = edge_to_route_.at(edge_id);
            std::string current_route_name = current_route == nullptr ? ""s : current_route->name;
            std::string current_stop_name = waiting_edge_to_stop_.count(edge_id)
                    ? waiting_edge_to_stop_.at(edge_id)->name : ""s;
            IntervalType current_type = current_route == nullptr ? IntervalType::WAITING : IntervalType::TRAVEL;
            double current_time = graph_.GetEdge(edge_id).weight;

            if (interval_type != current_type) {
                result.intervals.push_back({
                    std::exchange(interval_type, current_type),
                    std::exchange(interval_route_name, current_route_name),
                    std::exchange(waiting_stop_name, current_stop_name),
                    std::exchange(interval_time, current_time),
                    std::exchange(interval_stops_count, 1)
                });
            } else {
                interval_route_name = std::move(current_route_name);
                waiting_stop_name = std::move(current_stop_name);
                interval_time += current_time;
                ++interval_stops_count;
            }

        }

        return result;
    }

} // namespace transport_catalogue::service
