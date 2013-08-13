#ifndef SRL_ENVIRONMENT_H
#define SRL_ENVIRONMENT_H

#include "Value.h"
#include "Blocks.h"
#include "String.h"
#include "Hash.h"
#include "Parser.h"

namespace Srl {

    class Tree;
    class Node;

namespace Lib {

    template<class T> struct Link {
        size_t hash;
        T      field;

        Link(size_t hash_, const T& field_)
            : hash(hash_), field(field_) { }
    };

    template<class T>
    using Items = std::list<Lib::Link<T>, Lib::HeapAllocator<Lib::Link<T>>>;

    template<> struct HashSrl<size_t> {
        size_t operator()(size_t s) const
        {
            return Aux::hash_fnc((uint8_t*)&s, sizeof(size_t));
        }
    };

    template<> struct HashSrl<const void*> {
        size_t operator()(const void* v) const
        {
            size_t addr = (size_t)v;
            return HashSrl<size_t>()(addr);
        }
    };

    struct Environment {
        static const Encoding Str_Encoding = Encoding::UTF8;

        Environment(Tree& tree_) : tree(&tree_) { }

        Link<Node>*  create_node (Type type, const String& name);
        Link<Node>*  store_node  (Node& parent, const Node& node,  const String& name);
        Link<Value>* store_value (Node& parent, const Value& value, const String& name);

        size_t       hash_string  (const String& str);
        String       conv_string  (const String& str);

        std::pair<const String*, size_t> store_string (const String& str);

        Tree*                  tree;
        Heap                   heap;
        HTable<String, String> str_table;

        HTable<const void*, size_t>           shared_table_store   { 16 };
        HTable<size_t, std::shared_ptr<void>> shared_table_restore { 16 };

        std::vector<uint8_t>  str_buffer;
        std::vector<uint8_t>  type_buffer;

        Parser*   parser  = nullptr;
        Lib::Out* out     = nullptr;
        Lib::In*  in      = nullptr;
        bool      parsing = false;
        
        template<class T>
        Link<T>* create_link(Lib::Items<T>& lst, const T& val, const String& name);
    };
} }

#endif
