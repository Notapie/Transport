#include "json_reader.h"

#include <string>
#include <deque>
#include <vector>
#include <unordered_map>

namespace transport_catalogue::service {
    using namespace std::literals;

    JsonReader::JsonReader(TransportCatalogue& db) : db_(db) {}

    void JsonReader::ReadQueries(std::istream& input, std::ostream& output) {
        json::Document queries = json::Load(input);
        const json::Dict& root = queries.GetRoot().AsMap();

        if (root.count("base_requests"s)) {
            BaseRequests(root.at("base_requests"s).AsArray());
        }
        if (root.count("stat_requests"s)) {
            StatRequests(root.at("stat_requests"s).AsArray(), output);
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

            //остановки добавляем сразу, всё остальное копим
            db_.AddStop(stop_name,
                        request.at("latitude"s).AsDouble(),request.at("longitude"s).AsDouble());

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
            db_.AddBus(bus_name, stops, is_roundtrip ? '>' : '-');
        }
    }

    void JsonReader::StatRequests(const json::Array& requests, std::ostream& out) const {
        json::Array response;
        response.reserve(requests.size());
        for (const json::Node& request_node : requests) {
            const json::Dict& request = request_node.AsMap();

            std::string_view type = request.at("type"s).AsString();
            std::string_view name = request.at("name"s).AsString();
            int request_id = request.at("id"s).AsInt();
            if (type == "Bus"sv) {
                response.push_back(BusStat(name, request_id));
                continue;
            }
            response.push_back(StopStat(name, request_id));
        }
        json::Print(json::Document{response}, out);
    }

    json::Dict JsonReader::BusStat(std::string_view bus_name, int request_id) const {
        if (!db_.IsBusExists(bus_name)) {
            return json::Dict {
                {"request_id"s, request_id},
                {"error_message"s, "not found"s}
            };
        }
        domain::RouteInfo stat = db_.GetRouteInfo(bus_name);
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
        std::vector<std::string> buses = db_.GetStopBuses(stop_name);

        json::Array buses_array;
        buses_array.reserve(buses.size());
        json::Dict response {
            {"request_id"s, request_id},
        };

        for (std::string& bus : buses) {
            buses_array.emplace_back(std::move(bus));
        }
        response["buses"s] = std::move(buses_array);

        return response;
    }

} // namespace transport_catalogue::service
