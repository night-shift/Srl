#include "Srl/Srl.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

namespace {

    MemBlock conv_str(const String& in, vector<uint8_t>& buffer, Encoding encoding)
    {
        if(in.encoding() == encoding) {
            return MemBlock(in.data(), in.size());
        }
        auto size = Tools::conv_charset(encoding, in, buffer, true);

        return MemBlock(buffer.data(), size);
    }

    size_t hash_fnc(const MemBlock& block)
    {
        return murmur_hash2(block.ptr, block.size);
    }

    size_t hash_fnc(const String& str)
    {
        return murmur_hash2(str.data(), str.size());
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
    this->str_table  = move(s.str_table);

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

String Storage::conv_string(const String& str)
{
    return str.encoding() == Storage::Name_Encoding
        ? str
        : conv_str(str, this->str_buffer, Storage::Name_Encoding);
}

size_t Storage::hash_string(const String& str)
{
    return str.encoding() == Storage::Name_Encoding
        ? hash_fnc(str)
        : hash_fnc(conv_string(str));
}

template<class T>
Link<T>* Storage::create_link(const T& val, const String& name, Heap<Link<T>>& heap)
{
    static const String empty_str  = String(MemBlock(), Storage::Name_Encoding);

    size_t name_hash = 0;
    const String* str_ptr = nullptr;

    if(name.size() > 0) {

        auto conv = name.encoding() == Storage::Name_Encoding
            ? MemBlock(name.data(), name.size())
            : conv_str(name, this->str_buffer, Storage::Name_Encoding);

        name_hash = hash_fnc(conv);

        bool exists; String* ptr;
        tie(exists, ptr) = this->str_table.insert_hash(name_hash, conv);

        if(!exists) {
            if(ptr->block.can_store_local()) {
                ptr->block.move_to_local();

            } else {
                ptr->block.extern_data = Aux::copy(this->data_heap, conv).ptr;
            }
        }

        str_ptr = ptr;

    } else {
        name_hash = murmur_hash2(nullptr, 0);
        str_ptr = &empty_str;
    }

    auto* link = heap.create(name_hash, val);
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
        auto* link = this->create_link(node, name, this->node_heap);
        this->stored_nodes.push_back(&link->field);

        return link;
    }
}

Link<Node>* Storage::create_node(Tree& tree, Type type, const String& name)
{
    auto* link = this->create_link(Node(&tree, type), name, this->node_heap);
    this->stored_nodes.push_back(&link->field);

    return link;
}

Link<Value>* Storage::store_value(const Value& value, const String& name)
{
    auto* link = this->create_link(value, name, this->value_heap);
    auto& block = link->field.block;

    if(block.stored_local) {
        return link;
    }

    if(block.can_store_local()) {
        block.move_to_local();

    } else {
        block.extern_data = Aux::copy(this->data_heap, { block.extern_data, block.size }).ptr;
    }

    return link;
}

