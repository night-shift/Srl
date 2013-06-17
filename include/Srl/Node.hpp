#ifndef SRL_NODE_HPP
#define SRL_NODE_HPP

#include "Node.h"
#include "Tree.h"

namespace Srl {

    template<class T>
    Node& Node::insert(const String& name, const T& val)
    {
        Lib::Switch<T>::Insert(val, *this, name);
        return *this;
    }

    template<class Head, class... Tail>
    Node& Node::insert(const String& field_name, const Head& head, const Tail&... tail)
    {
        this->insert(field_name, head);
        return insert(tail...);
    }

    template<class T>
    Node& Node::insert(const String& field_name, const std::initializer_list<T>& elements)
    {
        auto* new_node = this->insert_node(Type::Array, field_name);
        for(auto& e : elements) {
           new_node->insert(e);
        }
        return *this;
    }

    template<class T>
    Node& Node::insert(const T& val)
    {
        return this->insert("", val);
    }

    template<>
    inline Node& Node::insert<Node>(const String& name, const Node& new_node)
    {
        this->insert_node(new_node, name);
        return *this;
    }

    template<>
    inline Node& Node::insert<Value>(const String& name, const Value& new_value)
    {
        this->insert_value(new_value, name);
        return *this;
    }

    inline Node& Node::insert()
    {
        return *this;
    }

    template<class T>
    T Node::unwrap() const
    {
        auto r = Ctor<T>::Create();
        this->paste(r);

        return std::move(r);
    }

    template<class T>
    void Node::paste(T& o) const
    {
        Lib::Switch<T>::Paste(o, *this);
    }

    template<class T>
    T Node::unwrap_field(const String& field_name) const
    {
        auto r = Ctor<T>::Create();
        this->paste_field(field_name, r);

        return std::move(r);
    }

    template<class T>
    T Node::unwrap_field(size_t index) const
    {
        auto r = Ctor<T>::Create();
        this->paste_field(index, r);

        return std::move(r);
    }

    template<class T>
    void Node::paste_field(const String& field_name, T& o) const
    {
        auto* item = this->find<T>(field_name);
        Lib::Switch<T>::Paste(o, *item, field_name);
    }

    template<class T>
    void Node::paste_field(size_t index, T& o) const
    {
        auto* item = this->find<T>(index);
        Lib::Switch<T>::Paste(o, *item, index);
    }

    template<class... Args>
    void Node::open_scope(void (*Insert)(Node& node, const Args&... args),
                          Type node_type, const String& name, const Args&... args)
    {
        if(this->just_parse) {
            this->tree->parse_value(Value(node_type), name);

            Insert(*this, args...);

            this->tree->parse_value(Value(Type::Scope_End), name);

        } else {
           auto* new_node = this->insert_node(node_type, name);
           Insert(*new_node, args...);
        }
    }

    template<class T, class ID>
    typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), Node*>::type
    Node::find(const ID& id) const
    {
        return this->node(id);
    }

    template<class T, class ID>
    typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), Value*>::type
    Node::find(const ID& id) const
    {
        return this->value(id);
    }

    template<class TParser>
    std::vector<uint8_t> Node::to_source(const TParser& parser)
    {
        TParser copy = parser;
        Lib::Out out;
        this->tree->set_output(copy, out);
        this->to_source();

        return out.extract();
    }

    template<class TParser>
    void Node::to_source(std::ostream& out_stream, const TParser& parser)
    {
        TParser copy = parser;
        Lib::Out out(out_stream);
        this->tree->set_output(copy, out);
        this->to_source();

        out.flush();
    }

    inline size_t Node::num_nodes() const
    {
        return this->nodes.size();
    }

    inline size_t Node::num_values() const
    {
        return this->values.size();
    }

    inline const String* Node::name() const
    {
        return this->name_ptr;
    }

    inline Type Node::type() const
    {
        return this->scope_type;
    }
}

#endif
