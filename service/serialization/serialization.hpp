#pragma once

#include <filesystem>
#include "service/map_renderer/map_renderer.h"
#include "service/transport_router/transport_router.h"
#include "transport_catalogue/transport_catalogue.h"

namespace transport_catalogue::service {

    struct SerializationSettings {
        std::filesystem::path file;
    };

    class Serialization {
    public:
        Serialization() = default;
        void UpdateSettings(SerializationSettings&& settings);

        void Serialize(const TransportCatalogue& db, const RouterSettings& routing_settings, const RenderSettings& render_settings) const;
        void Deserialize(TransportCatalogue& db, RouterSettings& settings, RenderSettings& renderSettings) const;

    private:
        SerializationSettings settings_;
    };

}
