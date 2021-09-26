#pragma once

#include "svg/svg.h"
#include "transport_catalogue/domain.h"
#include "geo/geo.h"

namespace transport_catalogue::service {

    struct RenderSettings {
        double width = 0.0;
        double height = 0.0;

        double padding = 0.0;

        double linew_width = 0.0;
        double stop_radius = 0.0;

        int bus_label_font_size = 0;
        geo::Coordinates bus_label_offsets;

        int stop_label_font_size = 0;
        geo::Coordinates stop_label_offsets;

        svg::Color underlayer_color;
        double underlayer_width = 0.0;

        std::vector<svg::Color> color_palette;
    };

    class MapRenderer {

    };

}
