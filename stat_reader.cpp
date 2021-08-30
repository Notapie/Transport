#include "stat_reader.h"

namespace transport_catalogue {

    using namespace detail;

    void StatReader::ReadQueries() const {
        using namespace std::string_literals;

        int queries_count = ReadLineWithNumber();

        std::deque<std::string> queries;

        for (int i = 0; i < queries_count; ++i) {
            queries.push_back(ReadLine());
        }
        for (std::string_view query : queries) {

            query = Trim(query);

            //Далее ищем первое слово, которое и будет типом запроса
            int64_t offset = query.find(' ');
            std::string_view type = query.substr(0, offset);

            //И сразу же исключаем его из нужных данных
            if (offset == query.npos) {
                continue;
            }
            query.remove_prefix(offset + 1);

            if (type == "Bus"s) {
                PrintBusHandler(query);
                continue;
            }

            if (type == "Stop"s) {
                PrintStopHandler(query);
            }

        }
    }

    void StatReader::PrintBusHandler(std::string_view bus_name) const {
        using namespace std::literals;

        if (!catalogue_.IsBusExists(bus_name)) {
            std::cout << "Bus "sv << bus_name << ": not found"sv << std::endl;
            return;
        }

        TransportCatalogue::RouteInfo info = catalogue_.GetBusRoute(bus_name);
        std::cout << "Bus "s << bus_name << ": "s;

        std::cout << info.total_stops << " stops on route, "s << info.uniq_stops << " unique stops, "s
        << info.real_length << " route length, "s << info.curvature << " curvature"s << std::endl;
    }

    void StatReader::PrintStopHandler(std::string_view stop_name) const {
        using namespace std::literals;

        if (!catalogue_.IsStopExists(stop_name)) {
            std::cout << "Stop "sv << stop_name << ": not found"sv << std::endl;
            return;
        }
        std::vector<std::string> info = catalogue_.GetStopBuses(stop_name);
        if (info.empty()) {
            std::cout << "Stop "sv << stop_name << ": no buses"sv << std::endl;
            return;
        }
        std::cout << "Stop "sv << stop_name << ": buses "sv;

        bool first = true;
        for (std::string_view bus : info) {
            if (!first) {
                std::cout << ' ';
            }
            first = false;
            std::cout << bus;
        }
        std::cout << std::endl;
    }

}
