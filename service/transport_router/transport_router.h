#pragma once

#include <vector>
#include <map>
#include <deque>
#include <optional>

#include "transport_catalogue/domain.h"
#include "geo/geo.h"
#include "router/graph.h"
#include "router/router.h"

namespace transport_catalogue::service {

    namespace detail {

        struct HubInfo {
            // Вершина ожидания автобуса, общий шлюз для всех маршрутов через остановку
            graph::VertexId input_vertex;
            // Обычные вершины маршрута. Назвал output потому что при пересадке они выполняют роль выходных
            std::vector<graph::VertexId> output_vertexes;
        };

    } // namespace detail


    class TransportRouter {

        enum class IntervalType {
            WAITING,
            TRAVEL
        };

        struct Interval {
            std::string route_name;
            IntervalType type = IntervalType::WAITING;
            double time = 0.0;
            size_t stops_count = 0;
        };

        struct Route {
            double total_time = 0.0;
            std::vector<Interval> intervals;
        };

    public:
        TransportRouter(const std::deque<domain::Bus>& buses);
        std::optional<Route> GetRoute(const domain::Stop* from, const domain::Stop* to) const;

    private:
        std::map<const domain::Stop*, detail::HubInfo> stop_to_hub_;
        std::map<graph::EdgeId, const domain::Bus*> edge_to_route_;

        graph::DirectedWeightedGraph<double> graph_;
        graph::Router<double> router_;

    };

} //namespace transport_catalogue::service
