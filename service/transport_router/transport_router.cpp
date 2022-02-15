#include "transport_router.h"

#include <unordered_set>

using namespace std::literals;

namespace transport_catalogue::service {

    template<typename It>
    size_t CountVertexes(It begin, It end) {
        //Считаем именно через маршруты, чтобы исключить остановки, через которые не ходят автобусы
        std::unordered_set<const domain::Stop*> uniq_stops;
        for (It it = begin; it != end; ++it) {
            for (const domain::Stop* stop : it->route) {
                uniq_stops.insert(stop);
            }
        }
        return uniq_stops.size() * 2;
    }

    TransportRouter::TransportRouter(const TransportCatalogue& catalogue) : catalogue_(catalogue) {}

    TransportRouter::TransportRouter(RouterSettings settings, const TransportCatalogue& catalogue)
    : settings_(settings), catalogue_(catalogue) {}

    void TransportRouter::UpdateSettings(RouterSettings settings) {
        settings_ = settings;
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
        if (settings_.bus_velocity <= 0) {
            throw std::logic_error("invalid bus velocity: \""s + std::to_string(settings_.bus_velocity) + "\""s);
        }
        size_t stops_count = bus.route.size();
        if (stops_count == 0) {
            return;
        }

        static auto compute_duration = [this](double distance) {
            return distance / (settings_.bus_velocity / 0.06);
        };

        for (size_t i = 0; i < stops_count - 1; ++i) {
            //Сначала нужно создать вершину остановки (или получить, если она уже была создана другим маршрутом)
            const Stop* current_stop_ptr = bus.route.at(i);
            graph::Edge current_hub = GetStopHub(current_stop_ptr);

            //Тут её нужно слинковать с последующими
            const Stop* temp_stop_ptr = current_stop_ptr; //промежуточная остановка для вычисления общего расстояния
            double temp_distance = 0.0;
            double temp_back_disatance = 0.0;
            for (size_t k = i + 1; k < stops_count; ++k) {
                size_t span_count = k - i;
                const Stop* next_stop_ptr = bus.route.at(k);
                graph::Edge next_hub = GetStopHub(next_stop_ptr);

                //Вычисляем время поездки
                temp_distance += catalogue_.GetRealLength(temp_stop_ptr, next_stop_ptr);
                double duration = compute_duration(temp_distance);

                //Создаём дугу поездки от current_hub.to до next_hub.from
                //Созданную дугу нужно сразу добавить в контейнер с информацией о ней
                edge_to_info_[graph_.AddEdge({current_hub.to, next_hub.from, duration})] = {
                        false,
                        duration,
                        span_count,
                        &bus,
                        next_stop_ptr
                };

                //А теперь то же самое, только наоборот в случае, если маршрут некольцевой
                if (bus.type == domain::RouteType::ONE_WAY) {
                    temp_back_disatance += catalogue_.GetRealLength(next_stop_ptr, temp_stop_ptr);
                    duration = compute_duration(temp_back_disatance);

                    edge_to_info_[graph_.AddEdge({next_hub.to, current_hub.from, duration})] = {
                            false,
                            duration,
                            span_count,
                            &bus,
                            current_stop_ptr
                    };
                }

                temp_stop_ptr = next_stop_ptr;
            }
        }
    }

    //Возвращает хаб остановки. Если его нет, создаёт и возвращает
    //Более ёмкого названия пока не придумал, но вроде и это подходит
    graph::Edge<double> TransportRouter::GetStopHub(const domain::Stop* stop) {
        if (stop_to_hub_.count(stop) > 0) {
            return graph_.GetEdge(stop_to_hub_.at(stop));
        }

        graph::Edge<double> new_edge = {
                vertex_counter_++,
                vertex_counter_++,
                static_cast<double>(settings_.bus_wait_time)
        };

        graph::EdgeId edge_id = stop_to_hub_[stop] = graph_.AddEdge(new_edge);

        edge_to_info_[edge_id] = {
                true,
                static_cast<double>(settings_.bus_wait_time),
                0,
                nullptr,
                stop
        };

        return new_edge;
    }

    std::optional<Route> TransportRouter::GetRoute(std::string_view from, std::string_view to) const {
        const Stop* from_stop_ptr = catalogue_.GetStop(from);
        const Stop* to_stop_ptr = catalogue_.GetStop(to);

        if (from_stop_ptr == nullptr || to_stop_ptr == nullptr || stop_to_hub_.count(from_stop_ptr) == 0 || stop_to_hub_.count(to_stop_ptr) == 0) {
            return std::nullopt;
        }

        graph::VertexId from_vertex = graph_.GetEdge(stop_to_hub_.at(from_stop_ptr)).from;
        graph::VertexId to_vertex = graph_.GetEdge(stop_to_hub_.at(to_stop_ptr)).from;

        std::optional<graph::Router<double>::RouteInfo> route = router_ptr_->BuildRoute(from_vertex, to_vertex);

        //Если маршрут построить не удалось, то возвращаем пустой optional
        if (!route) {
            return std::nullopt;
        }

        Route result;
        result.total_time = route->weight;
        result.intervals.reserve(route->edges.size());

        for (graph::EdgeId edge_id : route->edges) {
            result.intervals.push_back(edge_to_info_.at(edge_id));
        }

        return result;
    }

} // namespace transport_catalogue::service
