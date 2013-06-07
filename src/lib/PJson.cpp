#include "Srl/Srl.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

namespace {

    const function<void()> error = [] {
        throw Exception("Unable to parse JSON document. Index out of bounds.");
    };

    const In::Substitute substitute_unicode = [](In& src, vector<uint8_t>& buf, size_t idx) -> size_t{
        uint8_t hex_buffer[2];
        /* skip "\u" and read 4 hex digits */
        auto block = src.read_block(2 + 4, error);

        Tools::hex_to_bytes(hex_buffer, 2, block.ptr + 2, 4);

        return Tools::convert_charset(Encoding::UTF8, String(hex_buffer, 2, Encoding::UTF16), buf, true, idx);
    };

    template<class... chars>
    constexpr array<const char, sizeof...(chars) + 1> make(chars... chr)
    {
        return array<const char, sizeof...(chars) + 1> { { '\\', chr...  } };
    }

    void write_escape(const MemBlock& str, Out& out)
    {
        out.write_substitute(str,
            '\"', make('\"'), '\'', make('\''), '\\', make('\\'), '/', make('/'), '\n', make('n'),
            '\t', make('t'), '\r', make('r'), '\b', make('b'), '\f', make('f')
        );
    }

    MemBlock read_unescape(In& in, vector<uint8_t>& buffer)
    {
        auto len = in.read_substitue(error, '\"', buffer,
            '\"', make('\"'), '\'', make('\''), '\\', make('\\'), '/', make('/'), '\n', make('n'),
            '\t', make('t'), '\r', make('r'), '\b', make('b'), '\f', make('f'),
             substitute_unicode, make('u')
        );

        return { &buffer[0], len };
    }

    void insert_in_quotes(Out& out, const MemBlock& txt, bool escape = false)
    {
        out.write('\"');
        if(!escape) {
            out.write(txt);
        } else {
            write_escape(txt, out);
        }
        out.write('\"');
    }
}

/* Parse out ****************************************************/

void PJson::parse_out(const Value& value, const MemBlock& name, Out& out)
{
    auto type = value.type();
    this->insert_spacing(type, out);

    this->scope_closed = false;

    if(this->scope_type != Type::Null &&
       this->scope_type != Type::Container &&
       type != Type::Scope_End) {

        insert_in_quotes(out, name);
        out.write(':');

        if(this->insert_whitespace) {
            out.write(' ');
        }
    }

    if(TpTools::is_scope(type)) {

        auto c = type == Type::Container ? '[' : '{';

        out.write(c);
        this->scope_stack.push(type);
        this->scope_closed = true;
        this->scope_type = type;

        return;
    }

    if(type == Type::Scope_End) {

        assert(!this->scope_stack.empty());

        auto c = this->scope_type == Type::Container ? ']' : '}';

        out.write(c);
        this->scope_stack.pop();

        this->scope_type = !this->scope_stack.empty()
            ? this->scope_stack.top()
            : Type::Null;

        return;
    }

    if(TpTools::is_literal(type)) {
        out.write(value.data(), value.size());

    } else {
        insert_in_quotes(out, { value.data(), value.size() }, true);
    }
}

void PJson::insert_spacing(Type type, Out& out)
{
    if(!this->scope_closed && type != Type::Scope_End) {
        out.write(',');
    }
    if(!this->insert_whitespace) {
        return;
    }
    auto depth = type == Type::Scope_End
        ? this->scope_stack.size() - 1
        : this->scope_stack.size();

    if(this->scope_stack.size() > 0) {
        out.write('\n');
    }

    out.write_times(depth, '\t');
}

/* Parse in ****************************************************/

template<size_t C> void fill_table(uint8_t* table, uint8_t char_valid, uint8_t char_invalid)
{
    bool valid = (C >= '0' && C <= '9') ||
        C == 't' || C == 'r' || C == 'u' ||
        C == 'e' || C == 'f' || C == 'a' ||
        C == 'l' || C == 's' || C == 'T' ||
        C == 'F' || C == 'n' || C == 'N' ||
        C == '+' || C == '.' || C == '-' ||
        C == 'E' || C == 'i';

    table[C] = valid ? char_valid : char_invalid;

    fill_table<C + 1>(table, char_valid, char_invalid);
}

template<> void fill_table<256>(uint8_t*, uint8_t, uint8_t) { }

PJson::LiteralTable::LiteralTable()
{
    fill_table<0>(this->table, LiteralTable::Char_Valid, LiteralTable::Char_Invalid);
}

Parser::SourceSeg PJson::parse_in(In& source)
{
    State state;
    while(!state.complete) {

        /* literals can be 'un-quoted', values in 'square-brackets' don't have a name */
        if(!state.reading_value && this->scope_type != Type::Container) {

            source.move_until(error, '\"', ':', '{', '[', '}', ']');

        } else {
            source.move_while(error, ' ', ',', '\n', '\t', '\r');
        }

        bool move = true;
        char c = *source.pointer();
        switch(c) {
            case '\"': this->process_quote(source, state);
                       break;

            case ':' : if(state.reading_value) {
                           this->throw_exception(state, "Redundant semicolon.");
                        }
                        state.reading_value = true;
                        break;
            case '{' :
            case '}' :
            case '[' :
            case ']' : this->process_bracket(c, state);
                       break;

            default  : this->process_char(source, state, move);
        }

        if(move) {
            source.move(1, error);
        }
    }

    return SourceSeg(state.parsed_value, state.name, true);
}

void PJson::process_bracket(char bracket, State& state)
{
    Type type = bracket == '{' ? Type::Object
              : bracket == '[' ? Type::Container
              : Type::Scope_End;

    if(type == Type::Scope_End) {
        if(this->scope_stack.empty()) {
            this->throw_exception(state, "Redundant scope delimiter.");
        }

        this->scope_stack.pop();
        this->scope_type = !this->scope_stack.empty()
            ? this->scope_stack.top()
            : Type::Null;

    } else {
        this->scope_stack.push(type);
        this->scope_type = type;
    }

    state.parsed_value = Value(type);
    state.complete = true;
}

void PJson::process_quote(In& source, State& state)
{
    if(this->scope_type == Type::Null) {
        this->throw_exception(state, "Value not in a scope.");
    }
    /* skip opening quotation-mark */
    source.move(1, error);

    auto& buffer = state.name_processed ? this->value_buffer : this->name_buffer;
    MemBlock block = read_unescape(source, buffer);

    this->process_string(block, state);
}

void PJson::process_string(const MemBlock& content, State& state)
{
    /* container elements don't have a name */
    if(state.reading_value || this->scope_type == Type::Container) {
        state.parsed_value = Value(content, Encoding::UTF8);
        state.complete = true;

    } else {
        if(state.name_processed) {
            this->throw_exception(state, "Misssing \':\'.");
        }
        state.name = content;
        state.name_processed = true;
    }
}

/* values not in quotes are literals */
void PJson::process_char(In& source, State& state, bool& out_move)
{
    bool is_literal = this->lookup.table[*source.pointer()] == LiteralTable::Char_Valid;

    if(is_literal) {

        auto block = source.read_block_until(error, ',', ' ', '}', ']', '\n');

        this->process_literal(block, state);

        if(*source.pointer() == '}' || *source.pointer() == ']') {
            /* The bracket has a double meaning here, first it delimits the literal,
             * and second it closes the scope. So make sure it will be processed. */
            out_move = false;
        }
    }
}

void PJson::process_literal(const MemBlock& data, State& state)
{
    String wrap(data.ptr, data.size, Encoding::UTF8);

    auto conversion = Tools::string_to_type(wrap);

    if(!conversion.success) {
        throw_exception(state, String("Failed to convert string \'"
                        + wrap.unwrap<char>(false) + "\' to literal."));
    }

    auto* conv_data = conversion.type != Type::Null
        ? (uint8_t*)&conversion.int_value
        : nullptr;

    auto size = TpTools::get_size(conversion.type);

    if(size > 0) {
        assert(conv_data != nullptr);

        memcpy(this->conversion_buffer, conv_data, size);
        state.parsed_value = Value({ this->conversion_buffer, size }, conversion.type);

    } else {
        state.parsed_value = Value(MemBlock(), conversion.type);
    }

    state.complete = true;
}

void PJson::throw_exception(State& state, const String& info)
{
    auto name = state.name.size > 0
        ? string((const char*)state.name.ptr, state.name.size)
        : "";

    MemBlock content(state.parsed_value.data(), state.parsed_value.size());

    auto msg = "In field " + name + (content.ptr != nullptr && content.size > 0
            ? "\' at \'" + string((const char*)content.ptr, content.size) + "\'."
            : "." );

    throw Exception("Unable to parse JSON document. " + info.unwrap<char>(false) + " " + msg);
}

