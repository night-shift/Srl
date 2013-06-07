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

        return MemBlock(&buffer[0], size);
    }

    pair<size_t, MemBlock>
    process_name(const String& name, Heap& heap, vector<uint8_t>& buffer, bool store_data)
    {
        auto name_conv = convert_string(name, buffer, Storage::Name_Encoding);
        auto name_hash = hash_function(name_conv);

        if(store_data) {
            name_conv = heap.copy(name_conv);
        }

        return { name_hash, name_conv };
    }

    template<class T>
    Link<T>* create_link(uint8_t* mem, const T& val, const String& name, Heap& str_heap,
                         vector<uint8_t>& buffer, bool store_data)
    {
        auto pair = process_name(name, str_heap, buffer, store_data);
        auto* link = new(mem) Link<T>(
            pair.first,
            val,
            pair.second
        );

        return link;
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

    this->data_heap = move(s.data_heap);
    this->node_heap = move(s.node_heap);
    this->str_heap  = move(s.str_heap);

    return *this;
}

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

Link<Node>* Storage::store_node(const Node& node, Tree& tree, const String& name)
{
    if(node.tree != &tree) {
        auto* link = this->create_node(tree, node.scope_type, name);
        for(auto n : node.nodes) {
            link->field.insert_node(n->field, String(n->name, Storage::Name_Encoding));
        }
        for(auto v : node.values) {
            link->field.insert_value(v->field, String(v->name, Storage::Name_Encoding));
        }
        return link;

    } else {
        /* all the fields are already stored here */
        auto* mem = this->node_heap.alloc_type<Link<Node>>();
        auto* link = create_link(mem, node, name, this->str_heap, this->str_buffer, true);

        this->stored_nodes.push_back(&link->field);

        return link;
    }
}

Link<Node>* Storage::create_node(Tree& tree, Type type, const String& name, bool store_data)
{
    auto* mem = this->node_heap.alloc_type<Link<Node>>();
    auto* link = create_link(
        mem,
        Node(&tree, type),
        name, this->str_heap, this->str_buffer, store_data
    );

    this->stored_nodes.push_back(&link->field);

    return link;
}

Link<Value>* Storage::store_value(const Value& value, const String& name, bool store_data)
{
    auto* mem = this->data_heap.alloc_type<Link<Value>>();
    MemBlock data(value.data(), value.size());

    if(store_data && value.size() > 0) {
        data = this->data_heap.copy(data);
    }

    auto* link = create_link(
        mem,
        Value(data, value.type(), value.encoding()),
        name,
        this->str_heap, this->str_buffer, store_data
    );

    return link;
}

