#include "serialization.h"
#include <transport_catalogue.pb.h>
#include <fstream>

void transport_catalogue::service::Serialization::UpdateSettings(SerializationSettings&& settings) {
    settings_ = std::move(settings);
}

struct ColorSerializer {
    transport_schema::Color* color_schema_ptr;

    void operator()(svg::Rgb obj) const {
        // Делаем из структуры rgb массив байт
        char binary_rgb[sizeof(obj)];
        *reinterpret_cast<svg::Rgb*>(binary_rgb) = obj;

        // Закидываем этот массив в протосхему
        transport_schema::RgbFormat* rgb_format = color_schema_ptr->mutable_rgb_format();
        rgb_format->set_color_struct(binary_rgb, sizeof(obj));
    }
    void operator()(svg::Rgba obj) const {
        // Делаем из структуры rgb массив байт
        char binary_rgb[sizeof(obj)];
        *reinterpret_cast<svg::Rgba*>(binary_rgb) = obj;

        // Закидываем этот массив в протосхему
        transport_schema::RgbFormat* rgb_format = color_schema_ptr->mutable_rgb_format();
        rgb_format->set_color_struct(binary_rgb, sizeof(obj));

        // Ставим флаг, что это именно rgba
        rgb_format->set_is_rgba(true);
    }
    void operator()(const std::string& obj) const {
        transport_schema::StringFormat* string_format = color_schema_ptr->mutable_string_format();
        string_format->set_color_string(obj);
    }
    void operator()(std::monostate) const {
        return;
    }
};

void SerializeRenderSettings(transport_schema::RenderSettings* settings_schema_ptr,
                             const transport_catalogue::service::RenderSettings& render_settings) {
    settings_schema_ptr->set_width(render_settings.width);
    settings_schema_ptr->set_height(render_settings.height);
    settings_schema_ptr->set_padding(render_settings.padding);
    settings_schema_ptr->set_line_width(render_settings.line_width);
    settings_schema_ptr->set_stop_radius(render_settings.stop_radius);

    settings_schema_ptr->set_bus_label_font_size(render_settings.bus_label_font_size);
    transport_schema::Point* bus_point_ptr = settings_schema_ptr->mutable_bus_label_offsets();
    bus_point_ptr->set_x(render_settings.bus_label_offsets.x);
    bus_point_ptr->set_y(render_settings.bus_label_offsets.y);

    settings_schema_ptr->set_stop_label_font_size(render_settings.stop_label_font_size);
    transport_schema::Point* stop_point_ptr = settings_schema_ptr->mutable_stop_label_offsets();
    stop_point_ptr->set_x(render_settings.stop_label_offsets.x);
    stop_point_ptr->set_y(render_settings.stop_label_offsets.y);


    transport_schema::Color* underlayer_color_schema = settings_schema_ptr->mutable_underlayer_color();
    std::visit(ColorSerializer{underlayer_color_schema}, render_settings.underlayer_color);
    settings_schema_ptr->set_underlayer_width(render_settings.underlayer_width);

    // color palette
    for (const svg::Color color : render_settings.color_palette) {
        transport_schema::Color* palette_color_schema = settings_schema_ptr->add_color_palette();
        std::visit(ColorSerializer{palette_color_schema}, color);
    }
}

svg::Color DeserializeColor(const transport_schema::Color& color_schema) {
    if (color_schema.has_string_format()) {
        return color_schema.string_format().color_string();
    } else if (color_schema.has_rgb_format()) {
        const char* bytes = color_schema.rgb_format().color_struct().data();
        if (color_schema.rgb_format().is_rgba()) {
            return *reinterpret_cast<const svg::Rgba*>(bytes);
        } else {
            return *reinterpret_cast<const svg::Rgb*>(bytes);
        }
    } else {
        return {};
    }
};

void DeserializeRenderSettings(const transport_schema::RenderSettings& settings_schema_ptr,
                               transport_catalogue::service::RenderSettings& render_settings) {
    render_settings.width = settings_schema_ptr.width();
    render_settings.height = settings_schema_ptr.height();

    render_settings.padding = settings_schema_ptr.padding();

    render_settings.line_width = settings_schema_ptr.line_width();
    render_settings.stop_radius = settings_schema_ptr.stop_radius();

    render_settings.bus_label_font_size = settings_schema_ptr.bus_label_font_size();
    render_settings.bus_label_offsets = {
            settings_schema_ptr.bus_label_offsets().x(),
            settings_schema_ptr.bus_label_offsets().y(),
    };

    render_settings.stop_label_font_size = settings_schema_ptr.stop_label_font_size();
    render_settings.stop_label_offsets = {
            settings_schema_ptr.stop_label_offsets().x(),
            settings_schema_ptr.stop_label_offsets().y(),
    };

    render_settings.underlayer_color = DeserializeColor(settings_schema_ptr.underlayer_color());
    render_settings.underlayer_width = settings_schema_ptr.underlayer_width();

    render_settings.color_palette.reserve(settings_schema_ptr.color_palette_size());
    for (const transport_schema::Color color_schema : settings_schema_ptr.color_palette()) {
        render_settings.color_palette.push_back(DeserializeColor(color_schema));
    }
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

        transport_schema::Distance* distance_schema = base.add_distance();
        distance_schema->set_stop_index(stop_to_index.at(current_stop_name));
        distance_schema->set_dest_index(stop_to_index.at(dest_stop_name));
        distance_schema->set_length(length);
    }

    SerializeRenderSettings(base.mutable_render_settings(), render_settings);

    // TODO: Добавить сериализацию настроек роутинга

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

    DeserializeRenderSettings(base.render_settings(), renderSettings);

    // TODO: Добавить десериализацию настроек роутинга

}
