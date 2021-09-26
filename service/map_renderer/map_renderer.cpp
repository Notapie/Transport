#include "map_renderer.h"

namespace transport_catalogue::service {

    namespace detail {

        svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
            return {(coords.lng - min_lon_) * zoom_coeff_ + padding_,
                    (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
        }

    } // namespace detail

} // namespace transport_catalogue::service
