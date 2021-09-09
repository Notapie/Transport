#include "input_reader.h"

#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>


namespace transport_catalogue {

    namespace detail {

        std::string ReadLine(std::istream& input) {
            std::string s;
            getline(input, s);
            return s;
        }

        int ReadLineWithNumber(std::istream& input) {
            int result;
            input >> result;
            ReadLine(input);
            return result;
        }

        std::string_view DetachByDelimeter(std::string_view& raw_string, std::string_view delimeter) {
            size_t end = raw_string.npos;
            size_t pos = raw_string.find(delimeter);

            std::string_view result = raw_string.substr(0, pos);
            size_t to_remove = pos == end ? raw_string.size() : pos + delimeter.size();

            raw_string.remove_prefix(to_remove);
            return result;
        }

        char GetRouteType(std::string_view route) {
            int64_t end = route.npos;
            int64_t pos = route.find('-');
            if (pos != end) {
                return '-';
            }
            return '>';
        }

    }

    void InputReader::ReadQueries(std::istream& input) const {
        using namespace std::literals;
        int queries_count = detail::ReadLineWithNumber(input);

        //Тут просто хранилище исходных строк-запросов
        std::vector<std::string> queries;
        queries.reserve(queries_count);

        std::vector<std::string_view> stop_queries;
        std::vector<std::string_view> bus_queries;

        for (int i = 0; i < queries_count; ++i) {
            //Получаем ссылку на добавленный запрос
            std::string_view query = queries.emplace_back(detail::ReadLine(input));

            //Далее ищем первое слово, которое и будет типом запроса
            std::string_view type = detail::DetachByDelimeter(query, " "sv);

            //А тут заполняем данными без типа нужные контейнеры
            if (type == "Stop"s) {
                stop_queries.push_back(query);
                continue;
            }
            if (type == "Bus"s) {
                bus_queries.push_back(query);
            }
        }

        StopAddHandler(stop_queries);
        BusAddHandler(bus_queries);
    }

    void InputReader::StopAddHandler(std::vector<std::string_view>& queries) const {
        using namespace std::literals;
        std::unordered_map<std::string_view, std::deque<std::string_view>> stop_to_distance_queries;

        for (std::string_view query : queries) {
            std::string_view stop_name = detail::DetachByDelimeter(query, ": "sv);

            std::deque<std::string_view> props;
            while (!query.empty()) {
                props.push_back(detail::DetachByDelimeter(query, ", "sv));
            }

            double x = stod(std::string(props.at(0)));
            double y = stod(std::string(props.at(1)));

            //Если в параметрах есть расстояние до ближайших остановок, закидываем их в контейнер
            if (props.size() > 2) {
                for (size_t i = 2; i < props.size(); ++i) {
                    stop_to_distance_queries[stop_name].push_back(props[i]);
                }
            }

            //Дальше просто вызываем функцию добавления новой остановки класса справочника
            catalogue_.AddStop(stop_name, x, y);
        }
        InsertDistances(stop_to_distance_queries);
    }

    void InputReader::InsertDistances(
            std::unordered_map<std::string_view, std::deque<std::string_view>>& stop_to_distance_queries) const {
        using namespace std::literals;

        for (const auto& [stop, queries] : stop_to_distance_queries) {
            for (std::string_view query : queries) {
                int distance = stoi(std::string(detail::DetachByDelimeter(query, "m to "sv)));
                catalogue_.AddDistance(stop, query, distance);
            }
        }
    }

    void InputReader::BusAddHandler(const std::vector<std::string_view>& queries) const {
        using namespace std::literals;
        for (std::string_view query : queries) {
            std::string_view bus_name = detail::DetachByDelimeter(query, ": "sv);

            char type = detail::GetRouteType(query);
            std::string type_delimiter = " "s + type + " "s;

            std::vector<std::string_view> stops;
            size_t stops_count = std::count(query.begin(), query.end(), type) - 1;
            stops.reserve(stops_count);
            while (!query.empty()) {
                stops.push_back(detail::DetachByDelimeter(query, type_delimiter));
            }

            catalogue_.AddBus(bus_name, stops, type);
        }
    }

}
