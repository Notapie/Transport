#include "stat_reader.h"


void StatReader::ReadQueries() const {
    using namespace std::string_literals;

    int queries_count = ReadLineWithNumber();

    std::deque<std::string> queries;

    std::deque<std::string_view> bus_queries;

    for (int i = 0; i < queries_count; ++i) {

        std::string& query_ref = queries.emplace_back(ReadLine());
        std::string_view query_view = Trim(query_ref);

        //Далее ищем первое слово, которое и будет типом запроса
        int64_t offset = query_view.find(' ');
        std::string_view type = query_view.substr(0, offset);

        //И сразу же исключаем его из нужных данных
        if (offset == query_view.npos) {
            continue;
        }
        query_view.remove_prefix(offset + 1);

        if (type == "Bus"s) {
            bus_queries.push_back(query_view);
        }

    }
    for (const std::string_view query : bus_queries) {
        PrintBusHandler(query);
    }

}

void StatReader::PrintBusHandler(std::string_view bus_name) const {
    catalogue_.PrintBus(bus_name);
}
