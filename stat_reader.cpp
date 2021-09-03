#include "stat_reader.h"

#include <iostream>


namespace transport_catalogue {

    void StatReader::ReadQueries(std::istream& input, std::ostream& output) const {
        using namespace std::literals;

        int queries_count = detail::ReadLineWithNumber(input);

        std::vector<std::string> queries;
        queries.reserve(queries_count);

        for (int i = 0; i < queries_count; ++i) {
            queries.push_back(detail::ReadLine(input));
        }
        for (std::string_view query : queries) {

            //Далее ищем первое слово, которое и будет типом запроса
            std::string_view type = detail::DetachByDelimeter(query, " "sv);

            if (type == "Bus"s) {
                PrintBusHandler(query, output);
                continue;
            }

            if (type == "Stop"s) {
                PrintStopHandler(query, output);
            }

        }
    }

    void StatReader::PrintBusHandler(std::string_view bus_name, std::ostream& output) const {
        using namespace std::literals;

        if (!catalogue_.IsBusExists(bus_name)) {
            output << "Bus "sv << bus_name << ": not found"sv << std::endl;
            return;
        }

        TransportCatalogue::RouteInfo info = catalogue_.GetRouteInfo(bus_name);
        output << "Bus "s << bus_name << ": "s;

        output << info.total_stops << " stops on route, "s << info.uniq_stops << " unique stops, "s
        << info.real_length << " route length, "s << info.curvature << " curvature"s << std::endl;
    }

    void StatReader::PrintStopHandler(std::string_view stop_name, std::ostream& output) const {
        using namespace std::literals;

        if (!catalogue_.IsStopExists(stop_name)) {
            output << "Stop "sv << stop_name << ": not found"sv << std::endl;
            return;
        }
        std::vector<std::string> info = catalogue_.GetStopBuses(stop_name);
        if (info.empty()) {
            output << "Stop "sv << stop_name << ": no buses"sv << std::endl;
            return;
        }
        output << "Stop "sv << stop_name << ": buses "sv;

        bool first = true;
        for (std::string_view bus : info) {
            if (!first) {
                output << ' ';
            }
            first = false;
            output << bus;
        }
        output << std::endl;
    }

}
