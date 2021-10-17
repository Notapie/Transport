#pragma once

#include <iostream>
#include <string_view>

#include "json/json.h"
#include "transport_catalogue/transport_catalogue.h"
#include "service/map_renderer/map_renderer.h"

namespace transport_catalogue::service {

    class JsonReader {
    public:
        JsonReader(TransportCatalogue& db);

        // Читает из потока json и сохраняет его
        void ReadJson(std::istream& in);

        // Заполняет каталог из сохранённого json'а
        void FillCatalogue();

        // Обрабатывает запросы из сохранённого json'а и выводит результат в поток out
        void GetStats(std::ostream& out) const;

    private:
        TransportCatalogue& db_;
        MapRenderer map_renderer_;

        json::Document json_raw_;

        void HandleBaseRequests(const json::Array&);
        // Вспомогательные методы
        void BusAddRequests(const std::deque<const json::Dict*>& requests);
        void InsertDistances(const std::unordered_map<std::string_view, const json::Dict*>& requests);

        void HandleStatRequests(const json::Array&, std::ostream&) const;
        // Вспомогательные методы
        json::Dict GetBusStat(std::string_view bus_name, int request_id) const;
        json::Dict GetStopStat(std::string_view stop_name, int request_id) const;
        json::Dict RenderMap(int request_id) const;

    };

} // namespace transport_catalogue::service
