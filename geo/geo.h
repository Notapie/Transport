#pragma once

namespace geo {

    struct Coordinates {
        double lat = 0.0;
        double lng = 0.0;
    };

    double ComputeDistance(Coordinates from, Coordinates to);

} //namespace geo
