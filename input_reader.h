#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <unordered_map>

#include "transport_catalogue.h"


namespace transport_catalogue {

    namespace detail {

        std::vector<std::string_view> SplitBy(std::string_view text, char delimiter);
        std::string_view RemovePrefixWords(std::string_view text, int words_to_remove);
        std::string_view DetachByDelimeter(std::string_view& raw_string, std::string_view delimeter);
        std::string_view Trim(std::string_view text);
        char GetRouteType(std::string_view route);

        std::string ReadLine(std::istream& input);
        int ReadLineWithNumber(std::istream& input);

    }

    class InputReader {

    public:
        explicit InputReader(TransportCatalogue& catalogue) noexcept : catalogue_(catalogue) {}

        void ReadQueries(std::istream& input) const;


    private:
        TransportCatalogue& catalogue_;

        void StopAddHandler(std::vector<std::string_view>& queries) const;

        void InsertDistances(std::unordered_map<std::string_view, std::deque<std::string_view>>& queries) const;

        void BusAddHandler(std::vector<std::string_view>& queries) const;

    };

}
