#include "json_reader.h"

#include <string>

namespace transport_catalogue::service {

    JsonReader::JsonReader(const TransportCatalogue& db) : db_(db) {}

    void JsonReader::ReadQueries(std::istream& input, std::ostream& output) const {
        using namespace std::literals;
        json::Document queries = json::Load(input);
        const json::Dict& root = queries.GetRoot().AsMap();

        if (root.count("base_requests"s)) {
            BaseRequests(root.at("base_requests"s).AsArray());
        }
        if (root.count("stat_requests"s)) {
            StatRequests(root.at("stat_requests"s).AsArray(), output);
        }
    }

} // namespace transport_catalogue::service
