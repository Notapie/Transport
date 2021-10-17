#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {

    class Node;
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;
    using Data = std::variant<std::nullptr_t, Array, Dict, int, double, std::string, bool>;

    // Эта ошибка должна выбрасываться при ошибках парсинга JSON
    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

class Node final : private Data {
    public:

        using Data::Data;

        bool IsNull() const noexcept;
        bool IsInt() const noexcept;
        bool IsDouble() const noexcept;
        bool IsPureDouble() const noexcept;
        bool IsBool() const noexcept;
        bool IsString() const noexcept;
        bool IsArray() const noexcept;
        bool IsMap() const noexcept;

        int AsInt() const;
        double AsDouble() const;
        bool AsBool() const;
        const std::string& AsString() const;
        const Array& AsArray() const;
        const Dict& AsMap() const;

        bool operator==(const Node& other) const;
        bool operator!=(const Node& other) const;

        friend std::ostream& operator<<(std::ostream&, const Node& node);

    private:

        template<typename TypeHolds>
        bool IsTypeHolds() const {
            return std::holds_alternative<TypeHolds>(*this);
        }

    };

    std::ostream& operator<<(std::ostream& o, const Node& node);

    class Document {
    public:
        Document() = default;
        explicit Document(Node root);

        const Node& GetRoot() const;

        bool operator==(const Document& other) const;
        bool operator!=(const Document& other) const;

    private:
        Node root_;
    };

    Document Load(std::istream& input);

    void Print(const Document& doc, std::ostream& output);

}  // namespace json
