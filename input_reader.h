#pragma once

#include <string>
#include <vector>
#include <string_view>

#include "transport_catalogue.h"


std::vector<std::string_view> SplitBy(std::string_view text, char delimiter);
std::string_view Trim(std::string_view text);
char GetRouteType(std::string_view route);

std::string ReadLine();
int ReadLineWithNumber();

class InputReader {

public:
    explicit InputReader(TransportCatalogue& catalogue) noexcept : catalogue_(catalogue) {}

    void ReadQueries() const;


private:
    TransportCatalogue& catalogue_;

    void StopAddHandler(std::string_view query) const;

    void BusAddHandler(std::string_view query) const;

};
