#pragma once

#include <vector>
#include <map>
#include <deque>
#include <optional>
#include <memory>

#include "transport_catalogue/transport_catalogue.h"
#include "router/graph.h"
#include "router/router.h"

namespace transport_catalogue::service {

    struct RouterSettings {
        int bus_wait_time = 0;
        double bus_velocity = 0.0;
    };

    class TransportRouter {
    public:

        enum class IntervalType {
            WAITING,
            TRAVEL
        };

        struct Interval {
            IntervalType type = IntervalType::WAITING;
            std::string route_name;
            std::string stop_name;
            double time = 0.0;
            size_t stops_count = 0;
        };

        struct Route {
            double total_time = 0.0;
            std::vector<Interval> intervals;
        };

        TransportRouter(const TransportCatalogue& catalogue);
        TransportRouter(RouterSettings settings, const TransportCatalogue& catalogue);
        void UpdateSettings(RouterSettings settings);
        void BuildGraph();
        std::optional<Route> GetRoute(std::string_view from, std::string_view to) const;

    private:
        int bus_wait_time_ = 0; // минуты
        double bus_velocity_ = 0.0; // километры в час
        const TransportCatalogue& catalogue_;

        std::map<const domain::Stop*, graph::VertexId> stop_to_hub_;
        std::map<graph::EdgeId, const domain::Bus*> edge_to_route_;
        std::map<graph::EdgeId, const domain::Stop*> waiting_edge_to_stop_;
        size_t vertex_counter_ = 0;

        graph::DirectedWeightedGraph<double> graph_;
        std::unique_ptr<graph::Router<double>> router_ptr_;

        void AddBusRoute(const domain::Bus& bus);
        graph::VertexId CreateVertex(const domain::Stop*  stop);

    };

} //namespace transport_catalogue::service
