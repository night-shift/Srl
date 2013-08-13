#include "Srl/Srl.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

Tree::Tree(const String& name)
{
    this->env = unique_ptr<Environment>(new Environment(*this));
    this->root_node = &env->create_node(Type::Object, name)->field;
}

Tree::Tree(Tree&& g)
{
    *this = forward<Tree>(g);
}

Tree& Tree::operator= (Tree&& g)
{
    this->env       = move(g.env);
    this->root_node = g.root_node;
    this->env->tree = this;

    return *this;
}

void Tree::write(const Value& value, const String& field_name)
{
    this->write_conv(value, field_name, *this->env->parser);
}

void Tree::write(Type type, Parser& parser, Lib::Out& out, const function<void()>& store_switch)
{
    parser.clear();

    this->env->parser = &parser;
    this->env->out = &out;
    this->env->parsing = true;

    auto& name = this->root_node->name();
    this->write(Value(type), name);

    store_switch();

    this->write(Value(Type::Scope_End), this->root_node->name());
}

void Tree::set_output (Parser& parser, Lib::Out& out)
{
    parser.clear();
    this->env->parser = &parser;
    this->env->out = &out;
}

void Tree::read_source(Parser& parser, In& source)
{
    parser.clear();

    MemBlock name; Value val;
    tie(name, val) = parser.read(source);

    if(!TpTools::is_scope(val.type())) {
        throw Exception("Unable to parse source. Data malformed.");
    }

    auto* link = this->env->create_node(val.type(), name);
    this->root_node = &link->field;

    this->root_node->read_source(source, parser);
}

void Tree::read_source(Parser& parser, In& source, const function<void()>& restore_switch)
{
    parser.clear();

    this->env->in = &source;
    this->env->parser = &parser;
    this->env->parsing = true;

    MemBlock name; Value val;
    tie(name, val) = parser.read(source);

    if(!TpTools::is_scope(val.type())) {
        throw Exception("Unable to parse source. Data malformed.");
    }

    auto* link = this->env->create_node(val.type(), name);
    link->field.parsed = false;
    this->root_node = &link->field;

    restore_switch();
}

void Tree::write_conv(const Value& value, const String& val_name, Parser& parser)
{
    MemBlock name_conv;

    if(val_name.encoding() == Encoding::UTF8) {
        name_conv = MemBlock(val_name.data(), val_name.size());

    } else {
        auto& buffer = this->env->str_buffer;
        auto size = Tools::conv_charset(Encoding::UTF8, val_name, buffer, true);
        name_conv = MemBlock(buffer.data(), size);
    }

    auto type = value.type();

    bool no_conversion_needed =
        /* scope types don't need conversion */
        TpTools::is_scope(type) || type == Type::Scope_End ||
        /* strings must be UTF8 and escaped in text formats */
        (type == Type::String && parser.get_format() != Format::Text &&
         value.encoding() == Encoding::UTF8) ||
        /* binary types must be converted to string in text formats */
        (type != Type::String && parser.get_format() == Format::Binary);

    if(no_conversion_needed) {
        parser.write(value, name_conv, *this->env->out);

    } else {
        parser.write(this->conv_type(value), name_conv, *this->env->out);
    }
}

Value Tree::conv_type(const Value& value)
{
    size_t data_size = 0;
    auto& buffer = this->env->type_buffer;

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

Node& Tree::root()
{
    return *this->root_node;
}

void Tree::clear()
{
    this->env->clear();
    this->root_node = &this->env->create_node(Type::Object, "")->field;
}
