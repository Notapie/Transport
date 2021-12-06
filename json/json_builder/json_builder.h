#pragma once

#include "json/json.h"

namespace json {

    class Builder {

        class ArrayContext;
        class DictContext;
        class KeyContext;

        template<typename OwnerContext>
        class BaseContext {
        public:
            BaseContext(Builder& builder) : builder_(builder) {}
        protected:
            OwnerContext& Value(const Node& value) {
                builder_.Value(Node(value));
                return AsMainContext();
            }

            ArrayContext StartArray() {
                return builder_.StartArray();
            }

            DictContext StartDict() {
                return builder_.StartDict();
            }

            KeyContext Key(const std::string& key) {
                return builder_.Key(key);
            }

            Builder& EndArray() {
                return builder_.EndArray();
            }

            Builder& EndDict() {
                return builder_.EndDict();
            }
        private:
            Builder& builder_;
            OwnerContext& AsMainContext() {
                return static_cast<OwnerContext&>(*this);
            }
        };

        class ArrayContext : public BaseContext<ArrayContext> {
        public:
            using BaseContext::BaseContext;
            using BaseContext::Value;
            using BaseContext::StartArray;
            using BaseContext::EndArray;
            using BaseContext::StartDict;
        };

        class DictContext : public BaseContext<DictContext> {
        public:
            using BaseContext::BaseContext;
            using BaseContext::Key;
            using BaseContext::EndDict;
        };

        class KeyContext : public DictContext {
        public:
            using DictContext::DictContext;
            using BaseContext::Value;
            using BaseContext::StartArray;
            using BaseContext::StartDict;

        private:
            using BaseContext::Key;
            using BaseContext::EndDict;
        };

    public:
        Builder() = default;

        KeyContext Key(std::string);
        Builder& Value(Node&&);
        Builder& Value(const Node&);

        ArrayContext StartArray();
        Builder& EndArray();

        DictContext StartDict();
        Builder& EndDict();

        Node Build();

        Builder(const Builder&) = delete;
        Builder& operator=(const Builder&) = delete;

    private:
        Node root_;
        bool is_document_defined_ = false;
        bool is_document_finished_ = false;
        std::vector<Node*> nodes_stack_;

        std::string current_dict_key_;
        bool is_key_ready_ = false;

        template<typename T>
        void StartContainer() {
            using namespace std::literals;

            Node* new_node_ptr;
            if (!is_document_defined_) {
                root_ = T();
                is_document_defined_ = true;
                new_node_ptr = &root_;
            } else if (nodes_stack_.empty()) {
                throw std::logic_error("Starting container in the wrong context"s);
            } else {
                Node& current = *(nodes_stack_[nodes_stack_.size() - 1]);
                if (current.IsMap()) {
                    if (!is_key_ready_) {
                        throw std::logic_error("Failed attempt to insert into Dict! Key is not specified"s);
                    }
                    new_node_ptr = InsertIntoMap(current, current_dict_key_, T());
                    is_key_ready_ = false;
                } else if (current.IsArray()) {
                    new_node_ptr = InsertIntoArray(current, T());
                }
            }

            nodes_stack_.push_back(new_node_ptr);
        }

        template<typename T>
        void EndContainer() {
            using namespace std::literals;

            std::string container_name;
            bool is_dict = std::is_same_v<T, Dict>;
            bool is_array = std::is_same_v<T, Array>;

            if (is_array) {
                container_name = "Array"s;
            } else if (is_dict) {
                container_name = "Dict"s;
            }

            if (!is_document_defined_) {
                throw std::logic_error("Failed attempt to close "s + container_name + "! Document is not defined yet"s);
            } else if (is_document_finished_) {
                throw std::logic_error("Failed attempt to close "s + container_name + "! Document already finished"s);
            }

            Node& current_top = *(nodes_stack_[nodes_stack_.size() - 1]);

            if (!current_top.IsTypeHolds<T>()) {
                throw std::logic_error("Failed attempt to close "s + container_name
                + "! The order of ending is violated"s);
            } else if (is_dict && is_key_ready_) {
                throw std::logic_error("Failed attempt to close "s + container_name + "! An unclosed key \""s
                + current_dict_key_ + "\" found"s);
            }

            nodes_stack_.pop_back();
            if (nodes_stack_.empty()) {
                is_document_finished_ = true;
            }
        }

        //Методы для упрощения вставки значений в контейнеры
        Node* InsertIntoMap(Node& current_node, const std::string& key, Node&& value);
        Node* InsertIntoArray(Node& current_node, Node&& value);

    };

} // namespace json
