#include "input_reader.h"

#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <sstream>


std::string ReadLine() {
    std::string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}

//Разбиение текста с помощью разделителя
std::vector<std::string_view> SplitBy(std::string_view text, char delimiter) {
    std::vector<std::string_view> words;

    int64_t end = text.npos;
    while(true) {
        int64_t offset = text.find_first_not_of(delimiter);
        if (offset == end) break;

        text.remove_prefix(offset);
        int64_t next_delimiter = text.find(delimiter, 0);
        words.push_back(next_delimiter == end ? text : text.substr(0, next_delimiter));

        if (next_delimiter == end) break;
        text.remove_prefix(next_delimiter);
    }

    return words;
}

//Удаляет лишние пробелы слева и справа
std::string_view Trim(std::string_view text) {
    std::string_view result;

    int64_t end = text.npos;
    int64_t offset = text.find_first_not_of(' ');

    if (offset == end) {
        return text;
    }

    return text.substr(offset, text.find_last_not_of(' ') - offset + 1);
}

char GetRouteType(std::string_view route) {
    int64_t end = route.npos;
    int64_t pos = route.find('-');
    if (pos != end) {
        return '-';
    }
    return '>';
}

void InputReader::ReadQueries() const {
    using namespace std::string_literals;
    int queries_count = ReadLineWithNumber();

    //Тут просто хранилище исходных строк-запросов
    std::deque<std::string> queries;

    std::vector<std::string_view> stop_queries;
    std::vector<std::string_view> bus_queries;

    for (int i = 0; i < queries_count; ++i) {
        //Получаем ссылку на добавленный запрос
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

        //А тут заполняем данными без типа нужные контейнеры
        if (type == "Stop"s) {
            stop_queries.push_back(query_view);
            continue;
        }
        if (type == "Bus"s) {
            bus_queries.push_back(query_view);
        }
    }

    //И тпереь просто в нужном порядке вызываем обработчики
    for (const std::string_view query : stop_queries) {
        StopAddHandler(query);
    }
    for (const std::string_view query : bus_queries) {
        BusAddHandler(query);
    }
}

void InputReader::StopAddHandler(std::string_view query) const {
    std::string stop_name;

    //Делим запрос по символу ":"
    std::vector<std::string_view> name_to_coords = SplitBy(query, ':');
    stop_name = name_to_coords[0];

    std::vector<std::string_view> coords = SplitBy(name_to_coords[1], ',');

    double x = 0, y = 0;

    std::stringstream buffer;
    buffer << coords[0] << coords[1];

    buffer >> x;
    buffer >> y;

    //Дальше просто вызываем функцию добавления новой остановки класса справочника
    catalogue_.AddStop(stop_name, x, y);
}

void InputReader::BusAddHandler(std::string_view query) const {
    std::string_view bus_name;

    //Делим запрос по символу ":"
    std::vector<std::string_view> name_to_route = SplitBy(query, ':');
    bus_name = Trim(name_to_route[0]);

    char type = GetRouteType(name_to_route[1]);
    std::vector<std::string_view> stops = SplitBy(
            name_to_route[1],
            type
    );
    for (std::string_view& stop : stops) {
        stop = Trim(stop);
    }

    catalogue_.AddBus(bus_name, stops, type);
}
