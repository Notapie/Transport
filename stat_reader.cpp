#include "stat_reader.h"


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
    catalogue_.PrintBusRoute(bus_name);
}

void StatReader::PrintStopHandler(std::string_view stop_name) const {
    catalogue_.PrintStopBuses(stop_name);
}
