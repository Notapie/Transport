#include "json_reader.h"

#include <string>
#include <deque>
#include <vector>
#include <unordered_map>
#include <sstream>

namespace transport_catalogue::service {
    using namespace std::literals;

    double GetDoubleSetting(const json::Dict& list, const std::string& setting) {
        return list.count(setting) > 0 && list.at(setting).IsDouble() ? list.at(setting).AsDouble() : 0.0;
    }

    int GetIntSetting(const json::Dict& list, const std::string& setting) {
        return list.count(setting) > 0 && list.at(setting).IsInt() ? list.at(setting).AsInt() : 0;
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

    JsonReader::JsonReader(TransportCatalogue& db) : db_(db) {}

    void JsonReader::ReadQueries(std::istream& input, std::ostream& output) {
        json::Document json_raw = json::Load(input);
        const json::Dict& queries = json_raw.GetRoot().AsMap();

        if (queries.count("base_requests"s)) {
            BaseRequests(queries.at("base_requests"s).AsArray());
        }
        if (queries.count("render_settings"s)) {
            map_renderer_.UpdateSettings(ParseRenderSettings(queries.at("render_settings"s).AsMap()));
        }
        if (queries.count("stat_requests"s)) {
            StatRequests(queries.at("stat_requests"s).AsArray(), output);
        }
    }

    void JsonReader::BaseRequests(const json::Array& requests) {
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

    void JsonReader::StatRequests(const json::Array& requests, std::ostream& out) const {
        json::Array response;
        response.reserve(requests.size());
        for (const json::Node& request_node : requests) {
            const json::Dict& request = request_node.AsMap();

            std::string_view type = request.at("type"s).AsString();
            int request_id = request.at("id"s).AsInt();
            if (type == "Bus"sv) {
                response.push_back(BusStat(request.at("name"s).AsString(), request_id));
                continue;
            } else if (type == "Stop"sv) {
                response.push_back(StopStat(request.at("name"s).AsString(), request_id));
                continue;
            } else if (type == "Map"sv) {
                response.push_back(RenderMap(request_id));
            }
        }
        json::Print(json::Document{std::move(response)}, out);
    }

    json::Dict JsonReader::BusStat(std::string_view bus_name, int request_id) const {
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

    json::Dict JsonReader::StopStat(std::string_view stop_name, int request_id) const {
        if (!db_.IsStopExists(stop_name)) {
            return json::Dict {
                {"request_id"s, request_id},
                {"error_message"s, "not found"s}
            };
        }
        const std::set<std::string_view>& buses = db_.GetStopBuses(stop_name);

        json::Array buses_array;
        buses_array.reserve(buses.size());
        json::Dict response {
            {"request_id"s, request_id},
        };

        for (std::string_view bus : buses) {
            buses_array.push_back(std::string(bus));
        }
        response["buses"s] = std::move(buses_array);

        return response;
    }

    json::Dict JsonReader::RenderMap(int request_id) const {
        std::ostringstream oss;
        map_renderer_.Render(db_.GetBuses(), oss);
        return {
                {"map", oss.str()},
                {"request_id", request_id}
        };
    }

} // namespace transport_catalogue::service
