#include "json.h"

#include <unordered_map>

using namespace std;

namespace json {

    namespace {

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

        //Обработка перед выводом
        std::string OutPrepare(std::string_view raw_str) {
            std::string result(raw_str);
            ReplaceAll(result, "\\"sv, "\\\\"sv);
            ReplaceAll(result, "\""sv, "\\\""sv);
            ReplaceAll(result, "\n"sv, "\\n"sv);
            ReplaceAll(result, "\r"sv, "\\r"sv);
            ReplaceAll(result, "\t"sv, "\\t"sv);
            return result;
        }

        Node LoadNode(istream& input);

        Node LoadArray(istream& input) {
            Array result;
            char c;
            for (; input >> c && c != ']';) {
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }
            if (c != ']') {
                throw ParsingError("Failed attempt to parse Array! There is no second bracket"s);
            }
            return Node(move(result));
        }

        Node LoadNumber(std::istream& input) {
            using namespace std::literals;

            std::string parsed_num;

            // Считывает в parsed_num очередной символ из input
            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
            };

            // Считывает одну или более цифр в parsed_num из input
            auto read_digits = [&input, read_char] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
            };

            if (input.peek() == '-') {
                read_char();
            }
            // Парсим целую часть числа
            if (input.peek() == '0') {
                read_char();
                // После 0 в JSON не могут идти другие цифры
            } else {
                read_digits();
            }

            bool is_int = true;
            // Парсим дробную часть числа
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            // Парсим экспоненциальную часть числа
            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    // Сначала пробуем преобразовать строку в int
                    try {
                        return Node(std::stoi(parsed_num));
                    } catch (...) {
                        // В случае неудачи, например, при переполнении
                        // код ниже попробует преобразовать строку в double
                    }
                }
                return Node(std::stod(parsed_num));
            } catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        Node LoadBool(istream& input) {
            string str_value;
            str_value.reserve(5);
            str_value.resize(4);
            input.read(&str_value[0], 4);
            if (str_value[3] == 's') {
                str_value += static_cast<char>(input.get());
            }
            if (str_value != "true"sv && str_value != "false"sv) {
                throw ParsingError("Failed attempt to parse bool!"s);
            }
            return Node(str_value == "true"sv);
        }

        Node LoadNull(istream& input) {
            string str_value;
            str_value.resize(4);
            input.read(&str_value[0], 4);
            if (str_value != "null"sv) {
                throw ParsingError("Failed attempt to parse null!"s);
            }
            return Node();
        }


        Node LoadString(istream& input) {
            string line;
            char c;

            //new_seq - флаг, обозначающий, что началась последовательность управляющего символа
            bool new_seq = false;
            while (input) {
                input >> c;

                //сначала нужно проверить, нет ли у нас открытого начала последовательности
                if (new_seq) {
                    //если есть, то следующим символом должен быть один из списка
                    //если идёт какай-то другой символ, кидаем исключение
                    if (!(c == '\\' || c == '"' || c == 'n' || c == 'r' || c == 't')) {
                        throw ParsingError("Failed attempt to parse string!"s);
                    }

                    //в строку вставляем сразу нужный управляющий символ
                    static const std::unordered_map<char, char> pattern_to_char {
                            {'\\', '\\'},
                            {'"', '\"'},
                            {'n', '\n'},
                            {'r', '\r'},
                            {'t', '\t'},
                    };
                    line += pattern_to_char.at(c);

                    new_seq = false;
                    continue;
                }

                //если открытой последовательности у нас нет, нужно проверить, не начинается ли она
                if (c == '\\') {
                    new_seq = true;
                    continue;
                }

                //если открытой последовательности нет и текущий сивол - ", значит, парсинг строки завершён
                if (c == '\"') {
                    break;
                }

                line += c;
            }

            if (c != '"') {
                throw ParsingError("Failed attempt to parse string! There is no second quote"s);
            }

            return Node(move(line));
        }

        Node LoadDict(istream& input) {
            Dict result;
            char c;

            bool first = true;
            while (input >> c && c != '}') {
                //сначала проверям на наличие запятой, если элемент не первый
                if (!first) {
                    if (c != ',') {
                        throw ParsingError("Failed attempt to parse Dictionary!"s);
                    }
                    input >> c;
                }
                first = false;

                //тут проверяем на первую кавычку, чтобы убедиться, что ключом будет точно строка
                if (c != '"') {
                    throw ParsingError("Failed attempt to parse Dictionary!"s);
                }

                string key = LoadString(input).AsString();
                input >> c;

                //здесь проверяем, что ключ со значением разделены двоеточием
                if (c != ':') {
                    throw ParsingError("Failed attempt to parse Dictionary!"s);
                }

                result.insert({move(key), LoadNode(input)});
            }

            if (c != '}') {
                throw ParsingError("Failed attempt to parse Dictionary! There is no second bracket"s);
            }

            return Node(move(result));
        }

        Node LoadNode(istream& input) {
            char c;
            input >> c;

            if (c == '[') {
                return LoadArray(input);
            } else if (c == '{') {
                return LoadDict(input);
            } else if (c == '"') {
                return LoadString(input);
            }

            input.putback(c);
            if (c == 't' || c == 'f') {
                return LoadBool(input);
            } else if (c == 'n') {
                return LoadNull(input);
            } else {
                return LoadNumber(input);
            }
        }

        struct DataPrinter {
            std::ostream& out;

            void operator()(int value) {
                out << value;
            }

            void operator()(double value) {
                out << value;
            }

            void operator()(bool value) {
                out << std::boolalpha << value << std::noboolalpha;
            }

            void operator()(const Array& array) {
                out << '[';
                bool first = true;
                for (const Node& node : array) {
                    if (!first) {
                        out << ", "s;
                    }
                    first = false;
                    out << node;
                }
                out << ']';
            }

            void operator()(const Dict& dict) {
                out << '{';
                bool first = true;
                for (const auto& [key, node] : dict) {
                    if (!first) {
                        out << ", "s;
                    }
                    first = false;
                    out << "\""s << key << "\": "s << node;
                }
                out << '}';
            }

            void operator()(const std::string& value) {
                out << '"' << OutPrepare(value) << '"';
            }

            void operator()(std::nullptr_t) {
                out << "null"s;
            }
        };

    }  // namespace

    //Node
    const Array& Node::AsArray() const {
        if (!IsArray()) {
            throw std::logic_error("Failed attempt to parse node as array!"s);
        }
        return get<Array>(*this);
    }

    const Dict& Node::AsMap() const {
        if (!IsMap()) {
            throw std::logic_error("Failed attempt to parse node as map!"s);
        }
        return get<Dict>(*this);
    }

    int Node::AsInt() const {
        if (!IsInt()) {
            throw std::logic_error("Failed attempt to parse node as int!"s);
        }
        return get<int>(*this);
    }

    double Node::AsDouble() const {
        if (IsInt()) {
            return AsInt();
        }
        if (IsDouble()) {
            return get<double>(*this);
        }
        throw std::logic_error("Failed attempt to parse node as double!"s);
    }

    bool Node::AsBool() const {
        if (!IsBool()) {
            throw std::logic_error("Failed attempt to parse node as bool!"s);
        }
        return get<bool>(*this);
    }

    const string& Node::AsString() const {
        if (!IsString()) {
            throw std::logic_error("Failed attempt to parse node as string!"s);
        }
        return get<string>(*this);
    }

    //методы проверки типов в Node
    bool Node::IsNull() const noexcept {
        return IsTypeHolds<std::nullptr_t>();
    }

    bool Node::IsInt() const noexcept {
        return IsTypeHolds<int>();
    }

    bool Node::IsDouble() const noexcept {
        return IsTypeHolds<double>() || IsInt();
    }

    bool Node::IsPureDouble() const noexcept {
        return IsDouble() && !IsInt();
    }

    bool Node::IsBool() const noexcept {
        return IsTypeHolds<bool>();
    }

    bool Node::IsString() const noexcept {
        return IsTypeHolds<string>();
    }

    bool Node::IsArray() const noexcept {
        return IsTypeHolds<Array>();
    }

    bool Node::IsMap() const noexcept {
        return IsTypeHolds<Dict>();
    }

    //Сравнение и вывод Node
    bool Node::operator==(const Node& other) const {
        return static_cast<const Data&>(*this) == static_cast<const Data&>(other);
    }

    bool Node::operator!=(const Node& other) const {
        return !(*this == other);
    }

    std::ostream& operator<<(std::ostream& o, const Node& node) {
        std::visit(DataPrinter{o}, static_cast<const Data&>(node));
        return o;
    }

    //Document
    Document::Document(Node root) : root_(move(root)) {}

    const Node& Document::GetRoot() const {
        return root_;
    }

    bool Document::operator==(const Document& other) const {
        return root_ == other.root_;
    }

    bool Document::operator!=(const Document& other) const {
        return !(*this == other);
    }

    Document Load(istream& input) {
        return Document{LoadNode(input)};
    }

    void Print(const Document& doc, std::ostream& output) {
        output << doc.GetRoot();
    }

}  // namespace json
