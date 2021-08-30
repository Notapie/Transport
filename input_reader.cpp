#include "input_reader.h"

#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <sstream>

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
            int64_t end = text.npos;
            int64_t offset = text.find_first_not_of(' ');

            if (offset == end) {
                return {};
            }

            return text.substr(offset, text.find_last_not_of(' ') - offset + 1);
        }

        std::string_view RemovePrefixWords(std::string_view text, int words_to_remove) {
            for (int i = 0; i < words_to_remove; ++i) {
                text = Trim(text);
                text.remove_prefix(text.find(' '));
            }

            return Trim(text);
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
        std::unordered_map<std::string_view, std::deque<std::string_view>> stop_to_distance_queries;

        for (const std::string_view query : queries) {
            std::string_view stop_name;

            //Делим запрос по символу ":"
            std::vector<std::string_view> name_to_coords = detail::SplitBy(query, ':');
            stop_name = name_to_coords[0];

            std::vector<std::string_view> props = detail::SplitBy(name_to_coords[1], ',');

            double x = 0, y = 0;

            std::stringstream buffer;
            buffer << props[0] << props[1];

            buffer >> x;
            buffer >> y;

            //Если в параметрах есть расстояние до ближайших остановок, закидываем их в контейнер
            if (props.size() > 2) {
                for (size_t i = 2; i < props.size(); ++i) {
                    stop_to_distance_queries[stop_name].push_back(detail::Trim(props[i]));
                }
            }

            //Дальше просто вызываем функцию добавления новой остановки класса справочника
            catalogue_.AddStop(stop_name, x, y);
        }
        InsertDistances(stop_to_distance_queries);
    }

    void InputReader::InsertDistances(
            std::unordered_map<std::string_view, std::deque<std::string_view>>& stop_to_distance_queries) const {

        for (const auto& [stop, queries] : stop_to_distance_queries) {
            for (std::string_view query : queries) {
                std::stringstream buffer;

                int distance = 0;
                std::string_view destination;

                buffer << query;
                buffer >> distance;

                destination = detail::RemovePrefixWords(query, 2);
                catalogue_.AddDistance(stop, destination, distance);
            }
        }

    }

    void InputReader::BusAddHandler(std::vector<std::string_view>& queries) const {
        for (std::string_view query : queries) {
            std::string_view bus_name;

            //Делим запрос по символу ":"
            std::vector<std::string_view> name_to_route = detail::SplitBy(query, ':');
            bus_name = detail::Trim(name_to_route[0]);

            char type = detail::GetRouteType(name_to_route[1]);
            std::vector<std::string_view> stops = detail::SplitBy(name_to_route[1], type);
            for (std::string_view& stop : stops) {
                stop = detail::Trim(stop);
            }

            catalogue_.AddBus(bus_name, stops, type);
        }
    }

}
