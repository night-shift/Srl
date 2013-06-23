#include "Srl/Srl.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

Tree::Tree(Tree&& g)
{
    *this = move(g);
}

Tree& Tree::operator= (Tree&& g)
{
    this->storage   = move(g.storage);
    this->root_node = move(g.root_node);

    auto& nodes = this->storage.nodes();

    for(auto& n : nodes) {
        n->tree = this;
    }

    return *this;
}

void Tree::parse_value(const Value& value, const String& field_name)
{
    assert(this->temp_parser != nullptr && "Parser not set.");

    this->parse_out_convert(value, field_name, *this->temp_parser);
}

void Tree::parse_out(Parser& parser, Lib::Out& out, const function<void()>& store_switch)
{
    this->temp_parser = &parser;
    this->temp_stream = &out;

    this->root_node->just_parse = true;

    this->parse_value(Value(Type::Object), *this->root_node->name());

    store_switch();

    this->parse_value(Value(Type::Scope_End), *this->root_node->name());
}

void Tree::set_output (Parser& parser, Lib::Out& out)
{
    this->temp_parser = &parser;
    this->temp_stream = &out;
}

void Tree::parse_in(Parser& parser, In& source, bool just_parse)
{
    auto first_seg = parser.parse_in(source);
    auto& name = first_seg.name;
    auto& value = first_seg.value;

    if(!TpTools::is_scope(value.type())) {
        throw Exception("Unable to parse source. Data malformed.");
    }

    auto* link = this->storage.create_node(*this, value.type(), String(name, Encoding::UTF8), !just_parse);
    this->root_node = &link->field;
    this->root_node->just_parse = just_parse;

    this->root_node->parse_in(source, parser);
}

void Tree::parse_out_convert(const Value& value, const String& val_name, Parser& parser)
{
    MemBlock name_conv;

    if(val_name.encoding() == Encoding::UTF8) {
        name_conv = MemBlock(val_name.data(), val_name.size());

    } else {
        auto& buffer = this->storage.str_conv_buffer();
        auto size = Tools::convert_charset(Encoding::UTF8, val_name, buffer, true);
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
        parser.parse_out(value, name_conv, *this->temp_stream);

    } else {
        parser.parse_out(
            this->convert_type(value), name_conv, *this->temp_stream
        );
    }
}

Value Tree::convert_type(const Value& value)
{
    size_t data_size = 0;
    auto& buffer = this->storage.type_conv_buffer();

    if(value.type() == Type::String) {

        String wrap(value.data(), value.size(), value.encoding());
        data_size = Tools::convert_charset(Encoding::UTF8, wrap, buffer, true);

    } else if(TpTools::is_literal(value.type())) {

        data_size = Tools::type_to_string(value.type(), value.data(), buffer);

    } else {

        assert(value.type() == Type::Binary && "Value type invalid.");

        data_size = Tools::bytes_to_base64(value.data(), value.size(), buffer);
    }

    return Value( { buffer.data(), data_size }, value.type());

}

Node* Tree::root()
{
    return this->root_node;
}

void Tree::srl_resolve(Context& ctx)
{
    vector<uint8_t> source;

    if(ctx.mode() == Mode::Insert) {
        source = this->to_source<PSrl>();
    }

    auto wrap = BitWrap(
        source.data(), source.size(),
        [&source](size_t sz) { source.resize(sz); return source.data(); }
    );

    ctx("binary", wrap);
}
