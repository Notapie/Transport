#include "serialization.h"
#include <transport_catalogue.pb.h>
#include <fstream>

void transport_catalogue::service::Serialization::UpdateSettings(SerializationSettings&& settings) {
    settings_ = std::move(settings);
}

void transport_catalogue::service::Serialization::Serialize(const TransportCatalogue& db,
                                                            const RouterSettings& routing_settings,
                                                            const RenderSettings& render_settings) const {
    std::unordered_map<std::string_view, int32_t> stop_to_index;
    stop_to_index.reserve(db.GetStops().size());
    transport_schema::TransportCatalogue base;

    // Сначала запишем все остановки
    int32_t index = 0;
    for (const Stop& stop : db.GetStops()) {
        // Добавляем саму остановку
        transport_schema::Stop* stop_schema = base.add_stop();

        // Добавляем её название
        stop_schema->set_name(stop.name);

        // Добавляем её координаты
        transport_schema::Coords* stop_coords_schema = stop_schema->mutable_coords();
        stop_coords_schema->set_lat(stop.coords.lat);
        stop_coords_schema->set_lng(stop.coords.lng);

        // Записываем остановку в map для возможности в дальнейшем проще делать поиск индекса остановки по её названию
        stop_to_index[stop.name] = index++;
    }

    // Затем запишем все маршруты
    for (const Bus& bus : db.GetBuses()) {
        // Добавляем автобус
        transport_schema::Bus* bus_schema = base.add_bus();

        // Записываем его название
        bus_schema->set_name(bus.name);

        // Добавляем его маршрут, где остановки - это индексы из контейнера stop_to_inex
        transport_schema::Route* bus_route_schema = bus_schema->mutable_route();
        bus_route_schema->set_route_type(static_cast<int32_t>(bus.type));
        for (const Stop* stop_ptr : bus.route) {
            bus_route_schema->add_stop_index(stop_to_index.at(stop_ptr->name));
        }
    }

    // Теперь запишем дистанции между остановками
    for (const auto& [stops_pair, length] : db.GetDistances()) {
        std::string_view current_stop_name = stops_pair.first->name;
        std::string_view dest_stop_name = stops_pair.second->name;

        transport_schema::distance* distance_schema = base.add_distance();
        distance_schema->set_stop_index(stop_to_index.at(current_stop_name));
        distance_schema->set_dest_index(stop_to_index.at(dest_stop_name));
        distance_schema->set_length(length);
    }

    // TODO: Добавить сериализацию настроек

    // Теперь можно делать сериализацию в файл
    std::ofstream out_file {settings_.file, std::ios::binary};
    base.SerializeToOstream(&out_file);
}

void transport_catalogue::service::Serialization::Deserialize(TransportCatalogue& db, RouterSettings& settings,
                                                              RenderSettings& renderSettings) const {
    std::ifstream input_file {settings_.file, std::ios::binary};

    transport_schema::TransportCatalogue base;
    base.ParseFromIstream(&input_file);

    // Добавляем в справочник остановки
    for (const auto& stop_schema : base.stop()) {
        db.AddStop(stop_schema.name(), stop_schema.coords().lat(), stop_schema.coords().lng());
    }

    // Добавляем дистанции можеду остановками
    for (const auto& distance_schema : base.distance()) {
        std::string_view current_stop_name = base.stop(distance_schema.stop_index()).name();
        std::string_view dest_stop_name = base.stop(distance_schema.dest_index()).name();

        db.AddDistance(current_stop_name, dest_stop_name, distance_schema.length());
    }

    // Теперь маршруты
    for (const auto& bus_schema : base.bus()) {
        // Сделаем вектор из string_view
        std::vector<std::string_view> route;
        route.reserve(bus_schema.route().stop_index_size());
        for (const int32_t index : bus_schema.route().stop_index()) {
            route.emplace_back(base.stop(index).name());
        }

        db.AddBus(bus_schema.name(), route, static_cast<transport_catalogue::RouteType>(bus_schema.route().route_type()));
    }

    // TODO: Добавить десериализацию настроек

}
