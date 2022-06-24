#include "json_reader.h"

#include <string>
#include <deque>
#include <vector>
#include <unordered_map>
#include <sstream>

#include "json/json_builder/json_builder.h"

namespace transport_catalogue::service {
    using namespace std::literals;

    double GetDoubleSetting(const json::Dict& list, const std::string& setting) {
        return list.count(setting) > 0 && list.at(setting).IsDouble() ? list.at(setting).AsDouble() : 0.0;
    }

    int GetIntSetting(const json::Dict& list, const std::string& setting) {
        return list.count(setting) > 0 && list.at(setting).IsInt() ? list.at(setting).AsInt() : 0;
    }

    std::string GetStringSetting(const json::Dict& list, const std::string& setting) {
        return list.count(setting) > 0 && list.at(setting).IsString() ? list.at(setting).AsString() : ""s;
    }

    svg::Point GetPointSetting(const json::Dict& list, const std::string& setting) {
        if (list.count(setting) > 0) {
            const json::Array& offsets = list.at(setting).AsArray();
            return {
                    offsets.at(0).AsDouble(),
                    offsets.at(1).AsDouble(),
            };
        }
        return {};
    }

    svg::Color GetColor(const json::Node& color_node) {
        if (color_node.IsArray()) {
            const json::Array& props = color_node.AsArray();
            if (props.size() == 3) {
                return svg::Rgb {
                    static_cast<uint8_t>(props.at(0).AsInt()),
                    static_cast<uint8_t>(props.at(1).AsInt()),
                    static_cast<uint8_t>(props.at(2).AsInt()),
                };
            }
            return svg::Rgba {
                    static_cast<uint8_t>(props.at(0).AsInt()),
                    static_cast<uint8_t>(props.at(1).AsInt()),
                    static_cast<uint8_t>(props.at(2).AsInt()),
                    props.at(3).AsDouble(),
            };
        } else if (color_node.IsString()) {
            return color_node.AsString();
        }
        return {};
    }

    svg::Color GetColorSetting(const json::Dict& list, const std::string& setting) {
        if (list.count(setting) > 0) {
            return GetColor(list.at(setting));
        }
        return {};
    }

    std::vector<svg::Color> GetColorsVector(const json::Dict& list, const std::string& setting) {
        if (list.count(setting) == 0) {
            return {};
        }
        const json::Array& raw_colors = list.at(setting).AsArray();

        std::vector<svg::Color> result;
        result.reserve(raw_colors.size());

        for (const json::Node& color_node : raw_colors) {
            result.push_back(GetColor(color_node));
        }

        return result;
    }

    RenderSettings ParseRenderSettings(const json::Dict& settings) {
        return {
                GetDoubleSetting(settings, "width"s),
                GetDoubleSetting(settings, "height"s),

                GetDoubleSetting(settings, "padding"s),

                GetDoubleSetting(settings, "line_width"s),
                GetDoubleSetting(settings, "stop_radius"s),

                GetIntSetting(settings, "bus_label_font_size"s),
                GetPointSetting(settings, "bus_label_offset"s),

                GetIntSetting(settings, "stop_label_font_size"s),
                GetPointSetting(settings, "stop_label_offset"s),

                GetColorSetting(settings, "underlayer_color"s),
                GetDoubleSetting(settings, "underlayer_width"s),

                GetColorsVector(settings, "color_palette"s)
        };
    }

    RouterSettings ParseRoutingSettings(const json::Dict& settings) {
        return {
                GetIntSetting(settings, "bus_wait_time"s),
                GetDoubleSetting(settings, "bus_velocity"s)
        };
    }

    SerializationSettings ParseSerializationSettings(const json::Dict& settings) {
        return {
            GetStringSetting(settings, "file"s)
        };
    }

    JsonReader::JsonReader(TransportCatalogue& db) : db_(db), transport_router_(db) {}

    void JsonReader::ReadJson(std::istream& in) {
        json_raw_ = json::Load(in);
    }

    void JsonReader::FillCatalogue() {
        if (!json_raw_.GetRoot().IsMap()) {
            return;
        }
        const json::Dict& queries = json_raw_.GetRoot().AsMap();
        if (queries.count("base_requests"s)) {
            HandleBaseRequests(queries.at("base_requests"s).AsArray());
        }
        if (queries.count("render_settings"s)) {
            map_renderer_.UpdateSettings(ParseRenderSettings(queries.at("render_settings"s).AsMap()));
        }
        if (queries.count("routing_settings"s)) {
            transport_router_.UpdateSettings(
                    ParseRoutingSettings(queries.at("routing_settings"s).AsMap()));
            transport_router_.BuildGraph();
        }
        if (queries.count("serialization_settings"s)) {
            serialization_.UpdateSettings(
                ParseSerializationSettings(queries.at("serialization_settings"s).AsMap())
            );
        }
    }

    void JsonReader::GetStats(std::ostream& out) const {
        if (!json_raw_.GetRoot().IsMap()) {
            return;
        }
        const json::Dict& queries = json_raw_.GetRoot().AsMap();
        if (queries.count("stat_requests"s) > 0) {
            HandleStatRequests(queries.at("stat_requests"s).AsArray(), out);
        }
    }

    void JsonReader::SerializeData() const {
        serialization_.Serialize(db_, transport_router_, map_renderer_.GetSettings());
    }

    void JsonReader::DeserializeData() {
        RouterSettings router_settings;
        RenderSettings render_settings;

        serialization_.Deserialize(db_, router_settings, render_settings);

        map_renderer_.UpdateSettings(render_settings);

        // TODO: Обновить десериализованные настройки роутинга
    }

    void JsonReader::HandleBaseRequests(const json::Array& requests) {
        std::unordered_map<std::string_view, const json::Dict*> stop_to_distances;
        std::deque<const json::Dict*> bus_requests;

        for (const json::Node& node : requests) {
            const json::Dict& request = node.AsMap();

            if (request.at("type"s).AsString() == "Bus"s) {
                bus_requests.push_back(&request);
                continue;
            }

            std::string_view stop_name = request.at("name"s).AsString();

            // Остановки добавляем сразу, всё остальное копим
            db_.AddStop(stop_name,
                        request.at("latitude"s).AsDouble(), request.at("longitude"s).AsDouble());

            if (request.count("road_distances"s)) {
                stop_to_distances[stop_name] = &request.at("road_distances"s).AsMap();
            }
        }

        InsertDistances(stop_to_distances);
        BusAddRequests(bus_requests);
    }

    void JsonReader::InsertDistances(const std::unordered_map<std::string_view, const json::Dict*>& requests) {
        for (const auto& [name, dists_ptr] : requests) {
            for (const auto& [dest_name, destination] : *dists_ptr) {
                db_.AddDistance(name, dest_name, destination.AsInt());
            }
        }
    }

    void JsonReader::BusAddRequests(const std::deque<const json::Dict*>& requests) {
        for (const json::Dict* request : requests) {
            std::string_view bus_name = request->at("name"s).AsString();

            const json::Array& stops_array = request->at("stops"s).AsArray();
            std::vector<std::string_view> stops;
            stops.reserve(stops_array.size());
            for (const json::Node& stop : stops_array) {
                stops.emplace_back(stop.AsString());
            }

            bool is_roundtrip = request->at("is_roundtrip"s).AsBool();
            db_.AddBus(bus_name, stops, is_roundtrip ? RouteType::ROUND_TRIP : RouteType::ONE_WAY);
        }
    }

    void JsonReader::HandleStatRequests(const json::Array& requests, std::ostream& out) const {
        json::Builder response;
        response.StartArray();
        for (const json::Node& request_node : requests) {
            const json::Dict& request = request_node.AsMap();

            std::string_view type = request.at("type"s).AsString();
            int request_id = request.at("id"s).AsInt();
            if (type == "Bus"sv) {
                response.Value(GetBusStat(request.at("name"s).AsString(), request_id));
                continue;
            } else if (type == "Stop"sv) {
                response.Value(GetStopStat(request.at("name"s).AsString(), request_id));
                continue;
            } else if (type == "Map"sv) {
                response.Value(RenderMap(request_id));
                continue;
            } else if (type == "Route"sv) {
                std::string_view from = request.at("from"s).AsString();
                std::string_view to = request.at("to"s).AsString();
                response.Value(BuildRoute(request_id, from, to));
            }
        }
        json::Print(json::Document{std::move(response.EndArray().Build().AsArray())}, out);
    }

    json::Dict JsonReader::GetBusStat(std::string_view bus_name, int request_id) const {
        if (!db_.IsBusExists(bus_name)) {
            return json::Dict {
                {"request_id"s, request_id},
                {"error_message"s, "not found"s}
            };
        }
        RouteInfo stat = db_.GetRouteInfo(bus_name);
        return json::Dict {
                {"request_id"s, request_id},
                {"curvature"s, stat.curvature},
                {"route_length"s, stat.real_length},
                {"stop_count"s, (int) stat.total_stops},
                {"unique_stop_count"s, (int) stat.uniq_stops},
        };
    }

    json::Dict JsonReader::GetStopStat(std::string_view stop_name, int request_id) const {
        if (!db_.IsStopExists(stop_name)) {
            return json::Dict {
                {"request_id"s, request_id},
                {"error_message"s, "not found"s}
            };
        }
        const std::set<std::string_view>& buses = db_.GetStopBuses(stop_name);

        json::Builder response;
        response.StartDict()
            .Key("request_id"s).Value(request_id)
            .Key("buses"s).StartArray();

        for (std::string_view bus : buses) {
            response.Value(std::string(bus));
        }

        return response.EndArray().EndDict().Build().AsMap();
    }

    json::Dict JsonReader::RenderMap(int request_id) const {
        std::ostringstream oss;
        map_renderer_.Render(db_.GetBuses(), oss);
        return {
                {"map"s, oss.str()},
                {"request_id"s, request_id}
        };
    }

    json::Dict JsonReader::BuildRoute(int request_id, std::string_view from, std::string_view to) const {
        std::optional<Route> route = transport_router_.GetRoute(from, to);
        if (!route) {
            return {
                    {"request_id"s, request_id},
                    {"error_message"s, "not found"s}
            };
        }

        json::Builder response;
        response.StartDict().Key("request_id"s).Value(request_id)
            .Key("total_time").Value(route->total_time)
            .Key("items"s).StartArray();

        for (const EdgeInfo& interval : route->intervals) {
            response.StartDict()
            .Key("time"s).Value(interval.duration);
            if (!interval.is_waiting_edge) {
                response
                .Key("type"s).Value("Bus"s)
                .Key("bus"s).Value(interval.current_route->name)
                .Key("span_count"s).Value(static_cast<int>(interval.span_count));
            } else {
                response
                .Key("type"s).Value("Wait"s)
                .Key("stop_name"s).Value(interval.destination_stop->name);
            }
            response.EndDict();
        }

        return response.EndArray().EndDict().Build().AsMap();
    }

} // namespace transport_catalogue::service
