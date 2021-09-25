#include "svg.h"

namespace svg {

    using namespace std::literals;

    namespace detail {

        //Заменяет все подстроки pattern на подстроку replace
        void ReplaceAll(std::string& text, std::string_view pattern, std::string_view replace) {
            size_t end = text.npos;
            size_t replace_size = replace.size();
            size_t pattern_size = pattern.size();

            size_t pos = text.find(pattern);
            while (pos != end) {
                text.replace(pos, pattern_size, replace);
                pos = text.find(pattern, pos + replace_size);
            }
        }

        std::string ShieldString(std::string_view raw_string) {
            std::string result(raw_string);

            //сначала экранируем амперсанды
            ReplaceAll(result, "&"sv, "&amp;"sv);

            //потом уже всё остальное
            ReplaceAll(result, "\""sv, "&quot;"sv);
            ReplaceAll(result, "\'"sv, "&apos;"sv);
            ReplaceAll(result, "<"sv, "&lt;"sv);
            ReplaceAll(result, ">"sv, "&gt;"sv);

            return result;
        }

        void ColorPrinter::operator()(Rgb color) const {
            using namespace std::literals;
            out << "rgb("sv << unsigned(color.red) << ","sv << unsigned(color.green) << ","sv << unsigned(color.blue) << ")"sv;
        }

        void ColorPrinter::operator()(Rgba color) const {
            using namespace std::literals;
            out << "rgba("sv << unsigned(color.red) << ","sv << unsigned(color.green) << ","sv << unsigned(color.blue) << ","sv << color.opacity << ")"sv;
        }

        void ColorPrinter::operator()(const std::string& color) const {
            using namespace std::literals;
            out << color;
        }

        void ColorPrinter::operator()(std::monostate) const {
            using namespace std::literals;
            out << "none"sv;
        }

    } //namespace detail

    std::ostream& operator<<(std::ostream& o, StrokeLineCap line_cap) {
        using namespace std::literals;
        if (line_cap == StrokeLineCap::BUTT) {
            o << "butt"sv;
        } else if (line_cap == StrokeLineCap::ROUND) {
            o << "round"sv;
        } else if (line_cap == StrokeLineCap::SQUARE) {
            o << "square"sv;
        }
        return o;
    }

    std::ostream& operator<<(std::ostream& o, StrokeLineJoin line_join) {
        using namespace std::literals;
        if (line_join == StrokeLineJoin::ROUND) {
            o << "round"sv;
        } else if (line_join == StrokeLineJoin::MITER) {
            o << "miter"sv;
        } else if (line_join == StrokeLineJoin::ARCS) {
            o << "arcs"sv;
        } else if (line_join == StrokeLineJoin::BEVEL) {
            o << "bevel"sv;
        } else if (line_join == StrokeLineJoin::MITER_CLIP) {
            o << "miter-clip"sv;
        }
        return o;
    }

    std::ostream& operator<<(std::ostream& o, const Color& color) {
        std::visit(detail::ColorPrinter{o}, color);
        return o;
    }

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }

    // ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        RenderAttrs(out);
        out << " />"sv;
    }

    // ---------- Polyline ------------------

    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline points=\"";
        bool first = true;
        for (const Point& point : points_) {
            out << (first ? ""sv : " "sv);
            first = false;
            out << point.x << ","sv << point.y;
        }
        out << "\""sv;
        RenderAttrs(out);
        out << " />"sv;
    }

    // ---------- Text ------------------

    Text& Text::SetPosition(Point pos) {
        position_ = pos;
        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = std::move(font_family);
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = std::move(font_weight);
        return *this;
    }

    Text& Text::SetData(std::string data) {
        data_ = std::move(data);
        return *this;
    }

    void Text::RenderObject(const RenderContext &context) const {
        auto& out = context.out;
        out << "<text"sv;

        //Начальная позиция
        out << " x=\""sv
        << position_.x
        << "\" y=\""sv
        << position_.y
        << "\""sv;

        //сдвиг
        out << " dx=\""sv
        << offset_.x
        << "\" dy=\""sv
        << offset_.y
        << "\""sv;

        //размер шрифта
        out << " font-size=\""sv
        << font_size_
        << "\""sv;

        //толщина шрифта
        if (font_weight_ != ""s) {
            out << " font-weight=\""sv << font_weight_ << "\""sv;
        }

        //семейство шрифта
        if (font_weight_ != ""s) {
            out << " font-family=\""sv << font_family_ << "\""sv;
        }

        RenderAttrs(out);

        out << ">"sv << detail::ShieldString(data_) << "</text>"sv;
    }

    // ---------- Document ------------------

    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.emplace_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
        RenderContext ctx(out, 2, 2);

        for (const std::unique_ptr<Object>& object_ptr : objects_) {
            object_ptr->Render(ctx);
        }

        out << "</svg>"sv;
    }
}  // namespace svg
