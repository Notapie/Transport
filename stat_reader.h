#pragma once

#include "transport_catalogue.h"
#include "input_reader.h"


namespace transport_catalogue {

    class StatReader {

    public:
        explicit StatReader(TransportCatalogue& catalogue) noexcept : catalogue_(catalogue) {}

        void ReadQueries() const;

    private:
        TransportCatalogue& catalogue_;

        void PrintBusHandler(std::string_view bus_name) const;
        void PrintStopHandler(std::string_view stop_name) const;

    };

}
