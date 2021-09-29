#pragma once

#include <iostream>
#include <string_view>

#include "json/json.h"
#include "service/request_handler/request_handler.h"
#include "transport_catalogue/transport_catalogue.h"

namespace transport_catalogue::service {

    class JsonReader {
    public:
        JsonReader(TransportCatalogue& db);
        void ReadQueries(std::istream& input, std::ostream& output);

    private:
        TransportCatalogue& db_;

        void BaseRequests(const json::Array&);
        // Вспомогательные методы BaseRequests
        void BusAddRequests(const std::deque<const json::Dict*>& requests);
        void InsertDistances(const std::unordered_map<std::string_view, const json::Dict*>& requests);

        void StatRequests(const json::Array&, std::ostream&) const;
        // Вспомогательные методы StatRequests
        json::Dict BusStat(std::string_view bus_name, int request_id) const;
        json::Dict StopStat(std::string_view stop_name, int request_id) const;
        json::Dict RenderMap(int request_id) const;

    };

} // namespace transport_catalogue::service
