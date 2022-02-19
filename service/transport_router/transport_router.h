#pragma once

#include <vector>
#include <unordered_map>
#include <deque>
#include <optional>
#include <memory>

#include "transport_catalogue/transport_catalogue.h"
#include "router/graph.h"
#include "router/router.h"

namespace transport_catalogue::service {

    struct EdgeInfo {
        bool is_waiting_edge = false;
        double duration = 0.0;
        size_t span_count = 0;
        const domain::Bus* current_route = nullptr;
        const domain::Stop* destination_stop = nullptr;
    };

    struct Route {
        double total_time = 0.0;
        std::vector<EdgeInfo> intervals;
    };

    struct RouterSettings {
        int bus_wait_time = 0; // минуты
        double bus_velocity = 0.0; // километры в час
    };

    class TransportRouter {
    public:
        TransportRouter(const TransportCatalogue& catalogue);
        TransportRouter(RouterSettings settings, const TransportCatalogue& catalogue);
        void UpdateSettings(RouterSettings settings);
        void BuildGraph();
        std::optional<Route> GetRoute(std::string_view from, std::string_view to) const;

    private:
        RouterSettings settings_;
        const TransportCatalogue& catalogue_;

        // Тут EdgeId выполняет роль хаба, где from - это A', а to - это A
        std::unordered_map<const domain::Stop*, graph::EdgeId> stop_to_hub_;
        std::unordered_map<graph::EdgeId, EdgeInfo> edge_to_info_;

        size_t vertex_counter_ = 0;

        graph::DirectedWeightedGraph<double> graph_;
        std::unique_ptr<graph::Router<double>> router_ptr_;

        void AddBusRoute(const domain::Bus& from_stop_ptr);
        graph::Edge<double> GetStopHub(const domain::Stop* stop);

    };

} //namespace transport_catalogue::service
