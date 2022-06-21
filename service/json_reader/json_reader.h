#pragma once

#include <iostream>
#include <string_view>

#include "json/json.h"
#include "transport_catalogue/transport_catalogue.h"
#include "service/map_renderer/map_renderer.h"
#include "service/transport_router/transport_router.h"
#include "service/serialization/serialization.h"

namespace transport_catalogue::service {

    class JsonReader {
    public:
        JsonReader(TransportCatalogue& db);

        // Читает из потока json и сохраняет его
        void ReadJson(std::istream& in);

        // Заполняет каталог из сохранённого json'а
        void FillCatalogue();

        // Сериализует все настройки и саму базу какталога в файл имя которого передаётся через json методами ReadJson и FillCatalogue
        void SerializeData() const;

        // Десерализует все данные из файла
        void DeserializeData();

        // Обрабатывает запросы из сохранённого json'а и выводит результат в поток out
        void GetStats(std::ostream& out) const;

    private:
        TransportCatalogue& db_;
        Serialization serialization_;
        MapRenderer map_renderer_;
        TransportRouter transport_router_;

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
        json::Dict BuildRoute(int request_id, std::string_view from, std::string_view to) const;

    };

} // namespace transport_catalogue::service
