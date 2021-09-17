#pragma once

#include "transport_catalogue.h"
#include "input_reader.h"


namespace transport_catalogue {

    class StatReader {

    public:
        explicit StatReader(TransportCatalogue& catalogue) noexcept : catalogue_(catalogue) {}

        void ReadQueries(std::istream& input, std::ostream& output) const;

    private:
        const TransportCatalogue& catalogue_;

        void PrintBusHandler(std::string_view bus_name, std::ostream& output) const;
        void PrintStopHandler(std::string_view stop_name, std::ostream& output) const;

    };

}
