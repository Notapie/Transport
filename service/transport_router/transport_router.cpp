#include "transport_router.h"

namespace transport_catalogue::service {

    TransportRouter::TransportRouter(const std::deque<domain::Bus>& buses) {
        for (const domain::Bus& bus : buses) {
            AddBusRoute(bus);
        }

        router_ptr_ = std::make_unique<graph::Router<double>>(graph_);
    }

} // namespace transport_catalogue::service
