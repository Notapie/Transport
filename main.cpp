#include <fstream>
#include <iostream>
#include <string_view>
#include "transport_catalogue/transport_catalogue.h"
#include "service/json_reader/json_reader.h"

using namespace std::literals;
using namespace transport_catalogue;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    TransportCatalogue transport;
    service::JsonReader json_reader(transport);

    if (mode == "make_base"sv) {

        json_reader.ReadJson(std::cin);
        json_reader.FillCatalogue();
        json_reader.BuildGraph();
        json_reader.SerializeData();

    } else if (mode == "process_requests"sv) {

        json_reader.ReadJson(std::cin);
        json_reader.FillCatalogue();
        json_reader.DeserializeData();

        std::ofstream out_file {"output.json"s};
        if (!out_file) {
            std::cerr << "Could not open output file!"sv;
            return 0;
        }
        json_reader.GetStats(out_file);

    } else {
        PrintUsage();
        return 1;
    }
}