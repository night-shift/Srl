#include "Srl/Srl.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

namespace {

    Tools::HashFnv1a<MemBlock> hash_function;

    MemBlock convert_string(const String& in, vector<uint8_t>& buffer, Encoding encoding)
    {
        if(in.encoding() == encoding) {
            return MemBlock(in.data(), in.size());
        }
        auto size = Tools::convert_charset(encoding, in, buffer, true);

        return MemBlock(buffer.data(), size);
    }
}

Storage::~Storage()
{
    this->clear_nodes();
}

Storage::Storage(Storage&& s)
{
    *this = move(s);
}

Storage& Storage::operator= (Storage&& s)
{
    this->clear_nodes();
    this->stored_nodes = move(s.stored_nodes);

    this->data_heap  = move(s.data_heap);
    this->value_heap = move(s.value_heap);
    this->node_heap  = move(s.node_heap);
    this->str_heap   = move(s.str_heap);

    return *this;
}

/* For performance reasons Srl::Heap destructor only frees allocated memory segments,
 * no destructors being called, so clear the nodes' vectors by hand. */
void Storage::clear_nodes()
{
    for(auto n : this->stored_nodes) {
        n->nodes.~vector();
        n->values.~vector();
    }
}

size_t Storage::hash_string(const String& str)
{
    return str.encoding() == Storage::Name_Encoding
        ? hash_function({ str.data(), str.size() })
        : hash_function(convert_string(str, this->str_buffer, Storage::Name_Encoding));
}

template<class T>
Link<T>* Storage::create_link(const T& val, const String& name, Heap<Link<T>>& heap, bool store_name)
{
    static const String empty_str = String(MemBlock(), Storage::Name_Encoding);

     auto name_conv = name.encoding() == Storage::Name_Encoding
        ? MemBlock(name.data(), name.size())
        : convert_string(name, this->str_buffer, Storage::Name_Encoding);

    auto name_hash = hash_function(name_conv);

    const String* str_ptr = nullptr;

    if(store_name && name.size() > 0) {
        auto* mem = this->str_heap.get_mem(1);

        auto* ptr = new(mem) String(name_conv, Storage::Name_Encoding);
        if(ptr->block.can_store_local()) {
            ptr->block.move_to_local();

        } else {
            ptr->block.extern_data = copy_block(this->data_heap, name_conv).ptr;
        }
        str_ptr = ptr;

    } else {
        str_ptr = &empty_str;
    }

    auto* mem = heap.get_mem(1);

    auto* link = new(mem) Link<T>(name_hash, val);
    link->field.name_ptr = str_ptr;

    return link;
}

Link<Node>* Storage::store_node(const Node& node, Tree& tree, const String& name)
{
    if(node.tree != &tree) {
        auto* link = this->create_node(tree, node.scope_type, name);

        for(auto n : node.nodes) {
            link->field.insert_node(n->field, n->field.name());
        }
        for(auto v : node.values) {
            link->field.insert_value(v->field, v->field.name());
        }

        return link;

    } else {
        /* all the fields are already stored here */
        auto* link = this->create_link(node, name, this->node_heap, true);
        this->stored_nodes.push_back(&link->field);

        return link;
    }
}

Link<Node>* Storage::create_node(Tree& tree, Type type, const String& name, bool store_data)
{
    auto* link = this->create_link(Node(&tree, type), name, this->node_heap, store_data);
    this->stored_nodes.push_back(&link->field);

    return link;
}

Link<Value>* Storage::store_value(const Value& value, const String& name, bool store_data)
{
    auto* link = this->create_link(value, name, this->value_heap, store_data);

    auto& block = link->field.block;

    if(store_data) {
        if(block.can_store_local()) {
            block.move_to_local();

        } else {
            block.extern_data = copy_block(this->data_heap, { block.extern_data, block.size }).ptr;
        }
    }

    return link;
}

