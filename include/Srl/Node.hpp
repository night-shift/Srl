#ifndef SRL_NODE_HPP
#define SRL_NODE_HPP

#include "Node.h"
#include "Tree.h"

namespace Srl {

    template<class T>
    Node& Node::insert(const String& field_name, const T& val)
    {
        Lib::Switch<T>::Insert(val, *this, field_name);
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
        auto& new_node = this->insert_node(Type::Array, field_name);
        for(auto& e : elements) {
           new_node.insert(e);
        }
        return *this;
    }

    template<class T>
    Node& Node::insert(const T& val)
    {
        return this->insert("", val);
    }

    template<>
    inline Node& Node::insert<Node>(const String& field_name, const Node& new_node)
    {
        this->insert_node(new_node, field_name);
        return *this;
    }

    template<>
    inline Node& Node::insert<Value>(const String& field_name, const Value& new_value)
    {
        this->insert_value(new_value, field_name);
        return *this;
    }

    inline Node& Node::insert()
    {
        return *this;
    }

    template<class T>
    T Node::unwrap()
    {
        auto r = Ctor<T>::Create();
        this->paste(r);

        return std::move(r);
    }

    template<class T>
    void Node::paste(T& o)
    {
        Lib::Switch<T>::Paste(o, *this);
    }

    template<class T, class ID>
    T Node::unwrap_field(const ID& fieldID)
    {
        auto r = Ctor<T>::Create();
        this->paste_field(fieldID, r);

        return std::move(r);
    }

    template<class T, class ID>
    typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), void>::type
    Node::paste_field (const ID& fieldID, T& o)
    {
        if(this->parsed) {
            Lib::Switch<T>::Paste(o, this->node(fieldID), fieldID);

        } else {
            auto itm = this->consume_node(true, fieldID);
            Lib::Switch<T>::Paste(o, itm, fieldID);
            itm.consume_scope();
        }
    }

    template<class T, class ID>
    typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), void>::type
    Node::paste_field (const ID& fieldID, T& o)
    {
        if(this->parsed) {
            Lib::Switch<T>::Paste(o, this->value(fieldID), fieldID);

        } else {
            auto itm = this->consume_value(true, fieldID);
            Lib::Switch<T>::Paste(o, itm, fieldID);
        }
    }

    template<class... Args>
    void Node::open_scope(void (*Insert)(Node& node, const Args&... args),
                          Type node_type, const String& scope_name, const Args&... args)
    {
        if(this->env->parsing) {
            this->env->write(Value(node_type), scope_name);

            Insert(*this, args...);

            this->env->write(Value(Type::Scope_End), scope_name);

        } else {
           auto& new_node = this->insert_node(node_type, scope_name);
           Insert(new_node, args...);
        }
    }

    template<class T>
    typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), Value>::type
    Node::consume_item()
    {
        return this->consume_value(false, 0);
    }

    template<class T>
    typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), Node>::type
    Node::consume_item()
    {
        return this->consume_node(false, 0);
    }

    template<class T>
    typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), Lib::Items<Value>&>::type
    Node::items()
    {
        return this->values;
    }

    template<class T>
    typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), Lib::Items<Node>&>::type
    Node::items()
    {
        return this->nodes;
    }

    template<class TParser>
    std::vector<uint8_t> Node::to_source(TParser&& parser)
    {
        std::vector<uint8_t> vec;
        this->to_source(vec, parser);
        parser.clear();

        return std::move(vec);
    }

    template<class TParser>
    void Node::to_source(Lib::Out::Source source, TParser&& parser)
    {
        this->env->set_output(parser, source);
        this->to_source();
        this->env->out.flush();
        parser.clear();
    }

    inline size_t Node::num_nodes() const
    {
        return this->nodes.size();
    }

    inline size_t Node::num_values() const
    {
        return this->values.size();
    }

    inline const String& Node::name() const
    {
        return *this->name_ptr;
    }

    inline Type Node::type() const
    {
        return this->scope_type;
    }
}

#endif
