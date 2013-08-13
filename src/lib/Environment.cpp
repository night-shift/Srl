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
        return Aux::hash_fnc(block.ptr, block.size);
    }

    size_t hash_fnc(const String& str)
    {
        return Aux::hash_fnc(str.data(), str.size());
    }
}

String Environment::conv_string(const String& str)
{
    return str.encoding() == Environment::Str_Encoding
        ? str
        : conv_str(str, this->str_buffer, Environment::Str_Encoding);
}

size_t Environment::hash_string(const String& str)
{
    return str.encoding() == Environment::Str_Encoding
        ? hash_fnc(str)
        : hash_fnc(conv_string(str));
}

pair<const String*, size_t> Environment::store_string(const String& str)
{
    static const String empty_str  = String(MemBlock(), Environment::Str_Encoding);
    static const size_t empty_hash = hash_fnc(empty_str);

    size_t hash;
    const String* str_ptr;

    if(str.size() > 0) {

        auto conv = conv_str(str, this->str_buffer, Environment::Str_Encoding);
        hash = hash_fnc(conv);

        bool exists; String* ptr;
        tie(exists, ptr) = this->str_table.insert_hash(hash, conv);

        if(!exists) {
            if(!ptr->block.try_store_local()) {
                ptr->block.extern_data = Aux::copy(this->heap, conv).ptr;
            }
        }

        str_ptr = ptr;

    } else {
        hash    = empty_hash;
        str_ptr = &empty_str;
    }

    return { str_ptr, hash };
}

template<class T>
Link<T>* Environment::create_link(Lib::Items<T>& lst, const T& val, const String& name)
{
    auto strp = this->store_string(name);
    lst.emplace_back(strp.second, val);
    auto& link = lst.back();
    link.field.name_ptr = strp.first;

    return &link;
}

Link<Node>* Environment::store_node(Node& parent, const Node& node, const String& name)
{
    if(node.env != this) {
        auto* link = this->create_link(parent.nodes, Node(this->tree, node.scope_type), name);

        for(auto n : node.nodes) {
            link->field.insert_node(n.field, n.field.name());
        }
        for(auto v : node.values) {
            link->field.insert_value(v.field, v.field.name());
        }

        return link;

    } else {
        return this->create_link(parent.nodes, node, name);
    }
}

Link<Node>* Environment::create_node(Type type, const String& name)
{
    auto strp = this->store_string(name);
    auto* link = this->heap.create<Link<Node>>(strp.second, Node(this->tree, type));
    link->field.name_ptr = strp.first;

    return link;
}

Link<Value>* Environment::store_value(Node& parent, const Value& value, const String& name)
{
    auto* link = this->create_link(parent.values, value, name);
    auto& block = link->field.block;

    if(!block.stored_local && !block.try_store_local()) {
        block.extern_data = Aux::copy(this->heap, { block.extern_data, block.size }).ptr;
    }

    return link;
}

