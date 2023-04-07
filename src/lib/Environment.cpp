#include "Srl/Srl.h"
#include "Srl/Lib.h"

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

    uint64_t hash_fnc(const MemBlock& block)
    {
        return Aux::hash_fnc(block.ptr, block.size);
    }

    uint64_t hash_fnc(const String& str)
    {
        return Aux::hash_fnc(str.data(), str.size());
    }
}


const String Environment::EmptyString = Srl::String();
const uint64_t EmptyStringHash = hash_fnc(String());


String Environment::conv_string(const String& str)
{
    return str.encoding() == Environment::Str_Encoding
        ? str
        : conv_str(str, this->str_buffer, Environment::Str_Encoding);
}

uint64_t Environment::hash_string(const String& str)
{
    return str.encoding() == Environment::Str_Encoding
        ? hash_fnc(str)
        : hash_fnc(conv_string(str));
}

pair<const String*, size_t> Environment::store_string(const String& str)
{
    if(str.size() < 1) {
        return { &Environment::EmptyString, EmptyStringHash };
    }

    auto conv = conv_str(str, this->str_buffer, Environment::Str_Encoding);
    auto hash = hash_fnc(conv);

    auto* str_ptr = this->str_table.get(conv);

    if(str_ptr) {
        return { str_ptr, hash };
    }

    String new_str(conv);

    if(!new_str.block.try_store_local()) {
        new_str.block.extern_data = Aux::copy(this->heap, conv).ptr;
    }

    str_ptr = this->str_table.insert(new_str, new_str).second;
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
    if(this->parsing) {

        Value scope_start = Value(node.scope_type);
        this->write(scope_start, name);

        for(auto& v : node.values) {
            this->write(v.field, v.field.name());
        }

        for(auto& n : node.nodes) {
            this->store_node(parent, n.field, n.field.name());
        }

        Value scope_end = Value(Type::Scope_End);
        this->write(scope_end, name);

        return create_node(node.scope_type, name);
    }

    if(node.env != this) {
        auto* link = this->create_link(parent.nodes, Node(this->tree, node.scope_type), name);

        for(auto& n : node.nodes) {
            link->field.insert_node(n.field, n.field.name());
        }
        for(auto& v : node.values) {
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

void Environment::write(const Value& value, const String& field_name)
{
    this->write_conv(value, field_name);
}

void Environment::set_input(Parser& parser_, In::Source source)
{
    parser_.clear();
    this->parser = &parser_;
    this->in.set(source);
}

void Environment::set_output(Parser& parser_, Lib::Out::Source source)
{
    parser_.clear();
    this->parser = &parser_;
    this->out.set(source);
}

void Environment::write_conv(const Value& value, const String& val_name)
{
    MemBlock name_conv;

    if(val_name.encoding() == Encoding::UTF8) {
        name_conv = MemBlock(val_name.data(), val_name.size());

    } else {
        auto& buffer = this->str_buffer;
        auto size = Tools::conv_charset(Encoding::UTF8, val_name, buffer, true);
        name_conv = MemBlock(buffer.data(), size);
    }

    auto type = value.type();

    bool no_conversion_needed =
        /* scope types don't need conversion */
        TpTools::is_scope(type) || type == Type::Scope_End ||
        /* strings must be UTF8 in text formats */
        (type == Type::String && parser->get_format() != Format::Text &&
         value.encoding() == Encoding::UTF8) ||
        /* binary types must be converted to string in text formats */
        (type != Type::String && parser->get_format() == Format::Binary);

    if(no_conversion_needed) {
        parser->write(value, name_conv, this->out);

    } else {
        parser->write(this->conv_type(value), name_conv, this->out);
    }
}

Value Environment::conv_type(const Value& value)
{
    size_t data_size = 0;
    auto& buffer = this->type_buffer;

    if(value.type() == Type::String) {

        String wrap(value.data(), value.size(), value.encoding());
        data_size = Tools::conv_charset(Encoding::UTF8, wrap, buffer, true);

    } else if(TpTools::is_scalar(value.type())) {

        data_size = Tools::type_to_str(value, buffer);

    } else {

        assert(value.type() == Type::Binary && "Value type invalid.");

        data_size = Tools::bytes_to_base64(value.data(), value.size(), buffer);
    }

    return Value( { buffer.data(), data_size }, value.type());
}

void Environment::clear()
{
    this->heap.clear();
    this->str_table.clear();
    this->shared_table_store.clear();
    this->shared_table_restore.clear();
}
