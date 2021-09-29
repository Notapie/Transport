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

        bool StopPtrComparator::operator()(const domain::Stop* lhs, const domain::Stop* rhs) const {
            return rhs->name > lhs->name;
        }

    } // namespace detail

    MapRenderer::MapRenderer(const RenderSettings& settings) : settings_(settings) {}

    MapRenderer::MapRenderer(RenderSettings&& settings) : settings_(std::move(settings)) {}

    void MapRenderer::UpdateSettings(const RenderSettings& settings) {
        settings_ = settings;
    }

    void MapRenderer::UpdateSettings(RenderSettings&& settings) {
        settings_ = std::move(settings);
    }

    void MapRenderer::Render(const std::deque<domain::Bus>& buses, std::ostream& out) const {
        //сначала сортируем автобусы и остановки, чтобы рендерить их в нужном порядке
        std::set<const domain::Stop*, detail::StopPtrComparator> sorted_stops;
        std::vector<const domain::Bus*> sorted_buses;
        sorted_buses.reserve(buses.size());

        for (const domain::Bus& bus : buses) {
            if (bus.route.empty()) {
                continue;
            }
            sorted_buses.push_back(&bus);
            for (const domain::Stop* stop_ptr : bus.route) {
                sorted_stops.insert(stop_ptr);
            }
        }

        std::sort(sorted_buses.begin(), sorted_buses.end(), [](const domain::Bus* lhs, const domain::Bus* rhs) {
            return rhs->name > lhs->name;
        });

        //создаём проектор
        detail::SphereProjector projector(sorted_stops.begin(), sorted_stops.end(),
                                          settings_.width, settings_.height, settings_.padding);

        //начинаем рендеринг
        svg::Document canvas;

        InsertLines(canvas, projector, sorted_buses);
        InsertRouteNames(canvas, projector, sorted_buses);
        InsertStopSymbols(canvas, projector, sorted_stops);
        InsertStopNames(canvas, projector, sorted_stops);

        canvas.Render(out);
    }

    /*
     * Дальше вспомогательные методы
     */

    void MapRenderer::InsertLines(svg::Document& canvas, const detail::SphereProjector& projector,
                                  const std::vector<const domain::Bus*>& sorted_buses) const {
        //отрисовка линий маршрутов
        int i = 0;
        for (const domain::Bus* bus_ptr : sorted_buses) {
            std::unique_ptr<svg::Polyline> route(std::make_unique<svg::Polyline>());
            if (!settings_.color_palette.empty()) {
                route->SetStrokeColor(
                        settings_.color_palette.at(i++ % settings_.color_palette.size())
                );
            }
            route->SetFillColor(svg::NoneColor).SetStrokeWidth(settings_.line_width)
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            for (const domain::Stop* stop_ptr : bus_ptr->route) {
                route->AddPoint(projector(stop_ptr->coords));
            }

            if (bus_ptr->type == domain::RouteType::REVERSIBLE && bus_ptr->route.size() > 1) {
                for (size_t k = bus_ptr->route.size() - 1; k >= 1; --k) {
                    route->AddPoint(projector(bus_ptr->route.at(k - 1)->coords));
                }
            }
            canvas.AddPtr(std::move(route));
        }
    }

    void MapRenderer::InsertRouteNames(svg::Document& canvas, const detail::SphereProjector& projector,
                                       const std::vector<const domain::Bus*>& sorted_buses) const {
        //отрисовка названий маршрутов
        int i = 0;
        for (const domain::Bus* bus_ptr : sorted_buses) {
            svg::Text bus_title_background;

            const domain::Stop* fist_stop_ptr = bus_ptr->route.at(0);
            bus_title_background.SetOffset(settings_.bus_label_offsets)
                    .SetFontSize(settings_.bus_label_font_size)
                    .SetFontFamily("Verdana").SetFontWeight("bold").SetData(bus_ptr->name)
                    .SetPosition(projector(fist_stop_ptr->coords));

            svg::Text bus_title(bus_title_background);
            if (!settings_.color_palette.empty()) {
                bus_title.SetFillColor(
                        settings_.color_palette.at(i++ % settings_.color_palette.size())
                );
            }

            bus_title_background.SetFillColor(settings_.underlayer_color)
                    .SetStrokeColor(settings_.underlayer_color)
                    .SetStrokeWidth(settings_.underlayer_width).SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            canvas.AddPtr(std::make_unique<svg::Text>(bus_title_background));
            canvas.AddPtr(std::make_unique<svg::Text>(bus_title));

            const domain::Stop* last_stop_ptr = bus_ptr->route.at(bus_ptr->route.size() - 1);
            if (fist_stop_ptr != last_stop_ptr && bus_ptr->type == domain::RouteType::REVERSIBLE) {
                bus_title_background.SetPosition(projector(last_stop_ptr->coords));
                bus_title.SetPosition(projector(last_stop_ptr->coords));

                canvas.AddPtr(std::make_unique<svg::Text>(std::move(bus_title_background)));
                canvas.AddPtr(std::make_unique<svg::Text>(std::move(bus_title)));
            }
        }
    }

    void MapRenderer::InsertStopSymbols(svg::Document& canvas, const detail::SphereProjector& projector,
                                        const std::set<const domain::Stop*,
                                        detail::StopPtrComparator>& sorted_stops) const {
        //отрисовка символов остановок
        for (const domain::Stop* stop_ptr : sorted_stops) {
            std::unique_ptr<svg::Circle> circle(std::make_unique<svg::Circle>());
            circle->SetCenter(projector(stop_ptr->coords))
                    .SetRadius(settings_.stop_radius).SetFillColor("white");
            canvas.AddPtr(std::move(circle));
        }
    }

    void MapRenderer::InsertStopNames(svg::Document& canvas, const detail::SphereProjector& projector,
                                      const std::set<const domain::Stop*,
                                      detail::StopPtrComparator>& sorted_stops) const {
        //отрисовка названий остановок
        for (const domain::Stop* stop_ptr : sorted_stops) {
            std::unique_ptr<svg::Text> stop_title_background(std::make_unique<svg::Text>());

            stop_title_background->SetOffset(settings_.stop_label_offsets)
                    .SetFontSize(settings_.stop_label_font_size)
                    .SetFontFamily("Verdana").SetData(stop_ptr->name)
                    .SetPosition(projector(stop_ptr->coords));

            std::unique_ptr<svg::Text> stop_title(std::make_unique<svg::Text>(*stop_title_background));
            stop_title->SetFillColor("black");

            stop_title_background->SetFillColor(settings_.underlayer_color)
                    .SetStrokeColor(settings_.underlayer_color).SetStrokeWidth(settings_.underlayer_width)
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            canvas.AddPtr(std::move(stop_title_background));
            canvas.AddPtr(std::move(stop_title));
        }
    }

} // namespace transport_catalogue::service
