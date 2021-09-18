#pragma once

#include "json/json.h"
#include "request_handler.h"
#include "transport_catalogue/transport_catalogue.h"

namespace transport_catalogue::service {

    class JsonReader {
    public:
        JsonReader(const TransportCatalogue& db);
        void ReadQueries(std::istream& input, std::ostream& output) const;

    private:
        const TransportCatalogue& db_;

        void BaseRequests(json::Array&) const;
        void StatRequests(json::Array&, std::ostream&) const;

    };

} // namespace transport_catalogue::service
