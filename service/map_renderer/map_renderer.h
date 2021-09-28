#pragma once

#include <algorithm>

#include "svg/svg.h"
#include "transport_catalogue/domain.h"
#include "geo/geo.h"

namespace transport_catalogue::service {

    namespace detail {

        inline const double EPSILON = 1e-6;
        bool IsZero(double value);

        class SphereProjector {
        public:
            template <typename StopInputIt>
            SphereProjector(StopInputIt stops_begin, StopInputIt stops_end, double max_width,
                            double max_height, double padding)
                    : padding_(padding) {
                if (stops_begin == stops_end) {
                    return;
                }

                const auto [left_it, right_it]
                = std::minmax_element(stops_begin, stops_end, [](const auto lhs, const auto rhs) {
                    return lhs->coords.lng < rhs->coords.lng;
                });
                min_lon_ = (*left_it)->coords.lng;
                const double max_lon = (*right_it)->coords.lng;

                const auto [bottom_it, top_it]
                = std::minmax_element(stops_begin, stops_end, [](const auto lhs, const auto rhs) {
                    return lhs->coords.lat < rhs->coords.lat;
                });
                const double min_lat = (*bottom_it)->coords.lat;
                max_lat_ = (*top_it)->coords.lat;

                std::optional<double> width_zoom;
                if (!IsZero(max_lon - min_lon_)) {
                    width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
                }

                std::optional<double> height_zoom;
                if (!IsZero(max_lat_ - min_lat)) {
                    height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
                }

                if (width_zoom && height_zoom) {
                    zoom_coeff_ = std::min(*width_zoom, *height_zoom);
                } else if (width_zoom) {
                    zoom_coeff_ = *width_zoom;
                } else if (height_zoom) {
                    zoom_coeff_ = *height_zoom;
                }
            }

            svg::Point operator()(geo::Coordinates coords) const;

        private:
            double padding_;
            double min_lon_ = 0;
            double max_lat_ = 0;
            double zoom_coeff_ = 0;
        };

        struct StopPtrComparator {
            bool operator()(const domain::Stop*lhs, const domain::Stop* rhs) const;
        };

    } // namespace detail

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
    public:
        MapRenderer() = default;
        MapRenderer(const RenderSettings& settings);

        void Render(const std::deque<domain::Bus>& buses, std::ostream& out) const;
        void UpdateSettings(const RenderSettings& settings);

    private:
        RenderSettings settings_;
    };

} // namespace transport_catalogue::service
