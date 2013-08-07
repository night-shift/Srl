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

        template<class T> T unwrap ();
        template<class T> void paste (T& o);

        template<class T>
        typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), void>::type
        paste_field (const String& field_name, T& o);

        template<class T>
        typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), void>::type
        paste_field (size_t index, T& o);

        template<class T>
        typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), void>::type
        paste_field (const String& field_name, T& o);

        template<class T>
        typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), void>::type
        paste_field (size_t index, T& o);

        template<class T> T unwrap_field (const String& field_name);
        template<class T> T unwrap_field (size_t index);

        Node& node (const String& name);
        Node& node (size_t index);

        Value& value (const String& name);
        Value& value (size_t index);

        std::vector<Node*>  find_nodes  (const String& name, bool recursive = false) const;
        std::vector<Value*> find_values (const String& name, bool recursive = false) const;

        std::vector<Node*>  all_nodes  (bool recursive = false) const;
        std::vector<Value*> all_values (bool recursive = false) const;

        bool has_node  (const String& name);
        bool has_value (const String& name);

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
        template<class T>
        using Items = std::vector<Lib::Link<T>*>;

        Node(Tree* tree_, Type type_ = Type::Object, bool parsed_ = true)
            : tree(tree_), scope_type(type_), parsed(parsed_) { }

        Node() : Node(nullptr) { }

        Tree*         tree;
        const String* name_ptr;
        Type          scope_type;
        bool          parsed;

        Items<Node>  nodes;
        Items<Value> values;

        template<class... Args>
        void open_scope (void (*Insert)(Node& node, const Args&... args),
                         Type node_type, const String& name, const Args&... args);

        std::pair<bool, size_t> insert_shared (const void* obj);
        std::pair<bool, std::shared_ptr<void>*>  find_shared (size_t key, const std::function<std::shared_ptr<void>(void)>& create);

        Node& insert_node  (const Node& node, const String& name);
        Node& insert_node  (Type type, const String& name);
        void  insert_value (const Value& value, const String& name);

        void  to_source   ();
        void  read_source (Lib::In& source, Parser& parser);

        void  consume_scope ();
        Node  consume_node  (bool throw_exception);
        Value consume_value (bool throw_exception); 

        Node  consume_node  (const String& name);
        Value consume_value (const String& name);


        template<class T>
        typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), Value>::type
        consume_item();

        template<class T>
        typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), Node>::type
        consume_item();

        template<class T>
        typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), Items<Value>&>::type
        items();

        template<class T>
        typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), Items<Node>&>::type
        items();
    };
}

#endif
