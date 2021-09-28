#include "map_renderer.h"

#include <set>

namespace transport_catalogue::service {

    namespace detail {

        bool IsZero(double value) {
            return std::abs(value) < EPSILON;
        }

        svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
            return {(coords.lng - min_lon_) * zoom_coeff_ + padding_,
                    (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
        }

        bool StopPtrComparator::operator()(const domain::Stop *lhs, const domain::Stop *rhs) const {
            return lhs->name < rhs->name;
        }

    } // namespace detail

    MapRenderer::MapRenderer(const RenderSettings& settings) : settings_(settings) {}

    void MapRenderer::UpdateSettings(const RenderSettings& settings) {
        settings_ = settings;
    }

} // namespace transport_catalogue::service
