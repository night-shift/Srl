#ifndef SRL_NODE_H
#define SRL_NODE_H

#include "Enums.h"
#include "Common.h"
#include "Value.h"
#include "In.h"
#include "Resolve.h"
#include "TpTools.hpp"

#include <functional>

namespace Srl {

    class Tree;
    class String;
    struct Parser;

    namespace Lib {
        template<class T> struct Link;
        class Storage;
    }

    class Node {

        friend class Tree;
        template<class T, class U>
        friend struct Lib::Switch;
        friend class Lib::Storage;

    public :
        Node(Tree& tree_) : Node(&tree_, Type::Object) { }

        template<class Head, class... Tail>
        Node& insert (const String& field_name, const Head& head, const Tail&... tail);

        template<class T>
        Node& insert (const String& field_name, const T& value);

        template<class T>
        Node& insert (const String& field_name, const std::initializer_list<T>& elements);

        template<class T>
        Node& insert (const T& value);

        inline Node& insert();

        template<class T> T unwrap () const;
        template<class T> void paste (T& o) const;

        template<class T> void paste_field (const String& field_name, T& o) const;
        template<class T> void paste_field (size_t index, T& o) const;

        template<class T> T unwrap_field (const String& field_name) const;
        template<class T> T unwrap_field (size_t index) const;

        Node& node (const String& name) const;
        Node& node (size_t index) const;

        Value& value (const String& name) const;
        Value& value (size_t index) const;

        std::vector<Node*>  find_nodes  (const String& name, bool recursive = false) const;
        std::vector<Value*> find_values (const String& name, bool recursive = false) const;

        std::vector<Node*>  all_nodes  (bool recursive = false) const;
        std::vector<Value*> all_values (bool recursive = false) const;

        bool has_node  (const String& name) const;
        bool has_value (const String& name) const;

        void forall_nodes  (const std::function<void(Node*)>& fnc, bool recursive = false) const;
        void forall_values (const std::function<void(Value*)>& fnc, bool recursive = false) const;

        void remove_node (Node* node);
        void remove_node (size_t index);
        void remove_node (const String& name);

        void remove_value (Value* value);
        void remove_value (size_t index);
        void remove_value (const String& name);

        inline size_t num_nodes()   const;
        inline size_t num_values()  const;
        inline Type   type()        const;
        inline const String& name() const;

        template<class TParser>
        std::vector<uint8_t> to_source(const TParser& parser = TParser());

        template<class TParser>
        void to_source(std::ostream& stream, const TParser& parser = TParser());

    private:
        Node(Tree* tree_, Type type_ = Type::Object, bool just_parse_ = false)
            : tree(tree_), scope_type(type_), just_parse(just_parse_) { }

        Node() : Node(nullptr) { }

        Tree*         tree;
        Type          scope_type;
        bool          just_parse;
        const String* name_ptr;

        std::vector<Lib::Link<Node>*>  nodes;
        std::vector<Lib::Link<Value>*> values;

        template<class... Args>
        void open_scope (void (*Insert)(Node& node, const Args&... args),
                         Type node_type, const String& name, const Args&... args);

        Node& insert_node  (const Node& node, const String& name);
        Node& insert_node  (Type type, const String& name);
        void  insert_value (const Value& value, const String& name);

        void to_source ();
        void parse_in  (Lib::In& source, Parser& parser);

        template<class T, class ID>
        typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), Value&>::type
        find(const ID& id) const;

        template<class T, class ID>
        typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), Node&>::type
        find(const ID& id) const;
    };
}

#endif
