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

void SerializeTransportRouter(transport_schema::TransportRouter* transport_router_schema
                              , const transport_catalogue::service::TransportRouter& transport_router
                              , const std::unordered_map<std::string_view, int32_t>& stop_to_index
                              , const std::unordered_map<std::string_view, int32_t>& bus_to_index) {
    // Сначала сериализуем настройки
    transport_router_schema->set_router_settings_bytes(
        reinterpret_cast<const char*>(&transport_router.GetSettings()),
        sizeof(transport_router.GetSettings())
    );

    // Теперь содержимое TransportRouter
    // Vertex counter
    transport_router_schema->set_vertex_counter(transport_router.GetVertexCounter());

    // stop_to_hub
    for (const auto& [stop_ptr, edge_id] : transport_router.GetStopToHub()) {
        int32_t stop_index = stop_to_index.at(stop_ptr->name);
        transport_schema::StopToHub& stop_to_hub_schema = *transport_router_schema->add_stop_to_hub();
        stop_to_hub_schema.set_stop_index(stop_index);
        stop_to_hub_schema.set_edge_id(edge_id);
    }

    // edge_to_info
    for (const auto& [edge_id, info] : transport_router.GetEdgeToInfo()) {
        transport_schema::EdgeToInfo& edge_to_info_schema = *transport_router_schema->add_edge_to_info();
        edge_to_info_schema.set_edge_id(edge_id);

        transport_schema::EdgeInfo& edge_info_schema = *edge_to_info_schema.mutable_edge_info();
        edge_info_schema.set_is_waiting_edge(info.is_waiting_edge);
        edge_info_schema.set_duration(info.duration);
        edge_info_schema.set_span_count(info.span_count);
        const int32_t dest_stop_index = info.destination_stop != nullptr
            ? stop_to_index.at(info.destination_stop->name) : -1;
        const int32_t current_bus_index = info.current_route != nullptr
            ? bus_to_index.at(info.current_route->name) : -1;
        edge_info_schema.set_destination_stop_index(dest_stop_index);
        edge_info_schema.set_current_route_index(current_bus_index);
    }

    // router
    transport_schema::RouteInternalDataLists& router_data_schema = *transport_router_schema->mutable_router_data();
    for (const auto& list : transport_router.GetRouter().GetInternalData()) {
        transport_schema::RouteInternalDataList& data_list_schema = *router_data_schema.add_route_internal_data_list();

        for (const auto& data : list) {
            transport_schema::RouteInternalData& data_schema = *data_list_schema.add_route_internal_data();

            if (data) {
                data_schema.set_is_init(true);
                data_schema.set_weight(data->weight);

                if (data->prev_edge) {
                    data_schema.mutable_prev_edge()->set_edge_id(*data->prev_edge);
                }
            } else {
                data_schema.set_is_init(false);
            }
        }
    }

    // graph
    transport_schema::GraphContainers& graph_containers_schema = *transport_router_schema->mutable_graph_containers();
    graph_containers_schema.set_edges_bytes(
        reinterpret_cast<const char*>(transport_router.GetGraph().GetEdgesData().data()),
        sizeof(graph::Edge<double>) * transport_router.GetGraph().GetEdgesData().size()
    );

    for (const auto& list : transport_router.GetGraph().GetIndicesLists()) {
        transport_schema::IndicesList& indices_list = *graph_containers_schema.add_indices_list();

        indices_list.set_indices_bytes(
            reinterpret_cast<const char*>(list.data()),
            sizeof(graph::EdgeId) * list.size()
        );
    }
}

void DeserializeTransportRouter(const transport_catalogue::TransportCatalogue& catalogue
                                , const transport_schema::TransportCatalogue& base
                              , transport_catalogue::service::TransportRouter& transport_router) {
    using namespace transport_catalogue::service;

    const transport_schema::TransportRouter& transport_router_schema = base.transport_router();

    // router settings
    const RouterSettings& settings = *reinterpret_cast<const RouterSettings*>(transport_router_schema.router_settings_bytes().data());

    // vertex counter
    size_t vertex_counter = transport_router_schema.vertex_counter();

    // stop_to_hub
    std::unordered_map<const transport_catalogue::domain::Stop*, graph::EdgeId> stop_to_hub;
    stop_to_hub.reserve(transport_router_schema.stop_to_hub_size());
    for (const transport_schema::StopToHub& stop_to_hub_schema : transport_router_schema.stop_to_hub()) {
        const int32_t stop_index = stop_to_hub_schema.stop_index();
        stop_to_hub[catalogue.GetStop(base.stop(stop_index).name())] = stop_to_hub_schema.edge_id();
    }

    // edge_to_info
    std::unordered_map<graph::EdgeId, EdgeInfo> edge_to_info;

    for (const transport_schema::EdgeToInfo& edge_to_info_schema : transport_router_schema.edge_to_info()) {
        const transport_schema::EdgeInfo& edge_info_schema = edge_to_info_schema.edge_info();

        const int32_t route_index = edge_info_schema.current_route_index();
        const int32_t destination_index = edge_info_schema.destination_stop_index();
        const transport_catalogue::Bus* current_route = route_index < 0
                ? nullptr
                : catalogue.GetBus(base.bus(route_index).name());
        const transport_catalogue::Stop* destination_stop = destination_index < 0
                ? nullptr
                : catalogue.GetStop(base.stop(destination_index).name());
        EdgeInfo edge_info {
            edge_info_schema.is_waiting_edge(),
            edge_info_schema.duration(),
            edge_info_schema.span_count(),
            current_route,
            destination_stop,
        };

        const uint32_t edge_id = edge_to_info_schema.edge_id();
        edge_to_info[edge_id] = edge_info;
    }

    // router
    const transport_schema::RouteInternalDataLists& router_lists_schema = transport_router_schema.router_data();
    graph::Router<double>::RoutesInternalData router_internal_data;
    router_internal_data.reserve(router_lists_schema.route_internal_data_list_size());
    for (const transport_schema::RouteInternalDataList& router_list_schema : router_lists_schema.route_internal_data_list()) {
        auto& router_data_list = router_internal_data.emplace_back();
        router_data_list.reserve(router_lists_schema.route_internal_data_list_size());

        for (const transport_schema::RouteInternalData& data_schema : router_list_schema.route_internal_data()) {
            auto& data = router_data_list.emplace_back();

            if (data_schema.is_init()) {
                graph::Router<double>::RouteInternalData internal_data {
                    data_schema.weight()
                };
                if (data_schema.has_prev_edge()) {
                    internal_data.prev_edge = data_schema.prev_edge().edge_id();
                }
                data = internal_data;
            }
        }
    }

    // graph
    const transport_schema::GraphContainers& graph_containers_schema = transport_router_schema.graph_containers();
    std::vector<graph::Edge<double>> graph_edges;
    const size_t graph_edges_size = graph_containers_schema.edges_bytes().size() / sizeof(graph::Edge<double>);
    graph_edges.reserve(graph_edges_size);

    for (size_t i = 0; i < graph_edges_size; ++i) {
        const auto* edge_ptr = reinterpret_cast<const graph::Edge<double>*>(graph_containers_schema.edges_bytes().data()) + i;
        graph_edges.push_back(*edge_ptr);
    }

    std::vector<std::vector<graph::EdgeId>> indices_lists;
    indices_lists.reserve(graph_containers_schema.indices_list_size());
    for (const transport_schema::IndicesList& indices_list_schema : graph_containers_schema.indices_list()) {
        std::vector<graph::EdgeId>& indices_list = indices_lists.emplace_back();
        const size_t indices_list_size = indices_list_schema.indices_bytes().size() / sizeof(graph::EdgeId);
        indices_list.reserve(indices_list_size);

        for (size_t i = 0; i < indices_list_size; ++i) {
            const auto* edge_id_ptr = reinterpret_cast<const graph::EdgeId*>(indices_list_schema.indices_bytes().data()) + i;
            indices_list.push_back(*edge_id_ptr);
        }
    }
}

void transport_catalogue::service::Serialization::Serialize(const TransportCatalogue& db,
                                                            const TransportRouter& transport_router,
                                                            const RenderSettings& render_settings) const {
    std::unordered_map<std::string_view, int32_t> stop_to_index;
    std::unordered_map<std::string_view, int32_t> bus_to_index;
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
    index = 0;
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

        bus_to_index[bus.name] = index++;
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
    SerializeTransportRouter(base.mutable_transport_router(), transport_router, stop_to_index, bus_to_index);

    // Теперь можно делать сериализацию в файл
    std::ofstream out_file {settings_.file, std::ios::binary};
    base.SerializeToOstream(&out_file);
}

void transport_catalogue::service::Serialization::Deserialize(TransportCatalogue& db, TransportRouter& transport_router,
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
    DeserializeTransportRouter(db, base, transport_router);
}
