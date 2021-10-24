#include "json_builder.h"

using namespace std::literals;

namespace json {

    Builder::KeyContext Builder::Key(std::string key) {
        if (!is_defined_ || is_finished_ || !nodes_stack_[nodes_stack_.size() - 1]->IsMap()) {
            throw std::logic_error("Key() can be called only when Dict construction started!"s);
        } else if (is_key_ready_) {
            throw std::logic_error("Double key \""s + key + "\" setting!"s);
        }
        current_dict_key_ = std::move(key);
        is_key_ready_ = true;
        return KeyContext(*this);
    }

    Builder& Builder::Value(Node&& value) {
        //Если объект не определён, значит, можно заменять корень на значение
        if (!is_defined_) {
            root_ = std::move(value);
            is_defined_ = true;
            is_finished_ = true;
            return *this;
        } else if (is_finished_) {
            //Если объект определён и при этом стэк пуст (объект завершён), значит, были вызваны все End*
            //либо был вызван метод Value(). А значит, нужно кинуть исключение
            throw std::logic_error("Calling Value() in the wrong context! Document already finished"s);
        }

        Node& current = *(nodes_stack_[nodes_stack_.size() - 1]);

        //Если стэк не пустой, нужно проверить, не является ли вершина словарём
        if (current.IsMap()) {
            if (!is_key_ready_) {
                throw std::logic_error("Failed attempt to insert into Dict! Key is not specified"s);
            }
            InsertIntoMap(current, current_dict_key_, std::move(value));
            is_key_ready_ = false;
            return *this;
        }

        //Если же мы дошли сюда, вершина должна быть массивом
        InsertIntoArray(current, std::move(value));
        return *this;
    }

    Builder& Builder::Value(const Node& value) {
        Value(Node(value));
        return *this;
    }

    Builder::ArrayContext Builder::StartArray() {
        StartContainer<Array>();
        return ArrayContext(*this);
    }

    Builder::DictContext Builder::StartDict() {
        StartContainer<Dict>();
        return DictContext(*this);
    }

    Builder& Builder::EndArray() {
        EndContainer<Array>();
        return *this;
    }

    Builder& Builder::EndDict() {
        EndContainer<Dict>();
        return *this;
    }

    Node Builder::Build() {
        if (!is_finished_ || !is_defined_) {
            throw std::logic_error("JSON is not finished"s);
        }
        return root_;
    }

    Node* Builder::InsertIntoMap(Node& current_node, const std::string& key, Node&& value) {
        auto [it, inserted] = current_node.AsMap().insert({key, std::move(value)});
        if (!inserted) {
            throw std::logic_error("Failed attempt to insert in Dictionary! Duplicate key \""s
                               + key + "\" have been found"s);
        }
        return &(it->second);
    }

    Node* Builder::InsertIntoArray(Node& current_node, Node&& value) {
        return &(current_node.AsArray().emplace_back(std::move(value)));
    }

} // namespace json
