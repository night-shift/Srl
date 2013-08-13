#ifndef SRL_NODE_HPP
#define SRL_NODE_HPP

#include "Node.h"
#include "Tree.h"

namespace Srl {

    inline Node::Node(Tree* tree, Type type_, bool parsed_)
        : env(tree->env.get()), scope_type(type_), parsed(parsed_),
          nodes(env->heap), values(env->heap) { }

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

    template<class T>
    T Node::unwrap_field(const String& field_name)
    {
        auto r = Ctor<T>::Create();
        this->paste_field(field_name, r);

        return std::move(r);
    }

    template<class T>
    T Node::unwrap_field(size_t index)
    {
        auto r = Ctor<T>::Create();
        this->paste_field(index, r);

        return std::move(r);
    }
  
    template<class T>
    typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), void>::type
    Node::paste_field (const String& field_name, T& o)
    {
        if(this->parsed) {
            Lib::Switch<T>::Paste(o, this->node(field_name), field_name);

        } else {
            auto itm = this->consume_node(field_name);
            Lib::Switch<T>::Paste(o, itm, field_name);
            itm.consume_scope();
        }
    }

    template<class T>
    typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), void>::type
    Node::paste_field (size_t index, T& o)
    {
        if(this->parsed) {
            Lib::Switch<T>::Paste(o, this->node(index), index);

        } else {
            auto itm = this->consume_node(true);
            Lib::Switch<T>::Paste(o, itm, index);
            itm.consume_scope();
        }
    }

    template<class T>
    typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), void>::type
    Node::paste_field (const String& field_name, T& o)
    {
        if(this->parsed) {
            Lib::Switch<T>::Paste(o, this->value(field_name), field_name);

        } else {
            auto itm = this->consume_value(field_name);
            Lib::Switch<T>::Paste(o, itm, field_name);
        }
    }

    template<class T>
    typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), void>::type
    Node::paste_field (size_t index, T& o)
    {
        if(this->parsed) {
            Lib::Switch<T>::Paste(o, this->value(index), index);

        } else {
            auto itm = this->consume_value(true);
            Lib::Switch<T>::Paste(o, itm, index);
        }
    }
 
    template<class... Args>
    void Node::open_scope(void (*Insert)(Node& node, const Args&... args),
                          Type node_type, const String& scope_name, const Args&... args)
    {
        if(this->env->parsing) {
            this->env->tree->write(Value(node_type), scope_name);

            Insert(*this, args...);

            this->env->tree->write(Value(Type::Scope_End), scope_name);

        } else {
           auto& new_node = this->insert_node(node_type, scope_name);
           Insert(new_node, args...);
        }
    }

    template<class T>
    typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), Value>::type
    Node::consume_item()
    {
        return this->consume_value(false);
    }

    template<class T>
    typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), Node>::type
    Node::consume_item()
    {
        return this->consume_node(false);
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
        Lib::Out out;
        this->env->tree->set_output(parser, out);
        this->to_source();

        return out.extract();
    }

    template<class TParser>
    void Node::to_source(std::ostream& out_stream, TParser&& parser)
    {

        Lib::Out out(out_stream);
        this->env->tree->set_output(parser, out);
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
