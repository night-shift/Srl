#ifndef SRL_STORAGE_H
#define SRL_STORAGE_H

#include "Node.h"
#include "Value.h"
#include "Blocks.h"
#include "String.h"
#include "Hash.h"

namespace Srl { namespace Lib {

    template<class T> struct Link {

        size_t hash;
        T      field;

        Link(size_t hash_, const T& field_)
            : hash(hash_), field(field_) { }
    };

    class Storage {

    public :
        static const Encoding Name_Encoding = Encoding::UTF8;

        Storage() { }
        ~Storage();
        Storage& operator= (Storage&& g);
        Storage(Storage&& g);

        Link<Node>*  store_node  (const Node& node, Tree& tree, const String& name);
        Link<Node>*  create_node (Tree& tree, Type type, const String& name);
        Link<Value>* store_value (const Value& value, const String& name);

        size_t hash_string       (const String& str);
        String conv_string       (const String& str);

        inline std::vector<uint8_t>&  str_conv_buffer();
        inline std::vector<uint8_t>&  type_conv_buffer();
        inline std::deque<Node*>&     nodes();

   private :
        Heap<Link<Value>>      value_heap;
        Heap<Link<Node>>       node_heap;
        Heap<uint8_t>          data_heap;
        HTable<String, String> str_table;

        std::deque<Node*>     stored_nodes;
        std::vector<uint8_t>  str_buffer;
        std::vector<uint8_t>  type_buffer;

        void clear_nodes();
        template<class T>
        Link<T>* create_link(const T& val, const String& name, Heap<Link<T>>& heap);
    };


} }

#endif
