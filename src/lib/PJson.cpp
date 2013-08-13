#include "Srl/Srl.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

namespace {

    const function<void()> error = [] {
        throw Exception("Unable to parse JSON document. Index out of bounds.");
    };

    const In::Substitute unescape_uc_literal = [](In& src, vector<uint8_t>& buf, size_t idx) -> size_t {
        uint8_t hex_buffer[2];
        /* skip "\u" and read 4 hex digits */
        auto block = src.read_block(2 + 4, error);

        Tools::hex_to_bytes(hex_buffer, 2, block.ptr + 2, 4);

        return Tools::conv_charset(Encoding::UTF8, String(hex_buffer, 2, Encoding::UTF16), buf, true, idx);
    };

    constexpr array<const char, 2> ar(const char c)
    {
        return array<const char, 2> { { '\\', c } };
    }

    void write_escape(const MemBlock& str, Out& out)
    {
        out.write_substitute(str,
            '\"', ar('\"'), '\'', ar('\''), '\\', ar('\\'), '/', ar('/'), '\n', ar('n'),
            '\t', ar('t'), '\r',  ar('r'), '\b',  ar('b'), '\f', ar('f')
        );
    }

    MemBlock read_unescape(In& in, vector<uint8_t>& buffer)
    {
        auto len = in.read_substitue(error, '\"', buffer,
            '\"', ar('\"'), '\'', ar('\''), '\\', ar('\\'), '/', ar('/'), '\n', ar('n'),
            '\t', ar('t'), '\r',  ar('r'), '\b',  ar('b'), '\f', ar('f'), unescape_uc_literal, ar('u')
        );

        return { buffer.data(), len };
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

    template<size_t C> void fill_table(uint8_t* table)
    {
        bool valid = (C >= '0' && C <= '9') ||
            C == 't' || C == 'r' || C == 'u' ||
            C == 'e' || C == 'f' || C == 'a' ||
            C == 'l' || C == 's' || C == 'T' ||
            C == 'F' || C == 'n' || C == 'N' ||
            C == '+' || C == '.' || C == '-' ||
            C == 'E' || C == 'i';

        table[C] = valid;

        fill_table<C + 1>(table);
    }

    template<> void fill_table<256>(uint8_t*) { }

    void skip_comment(Lib::In& source)
    {
        assert(*source.pointer() == '/');

        if(!source.try_peek(1)) {
            return;
        }

        if(*(source.pointer() + 1) == '*') {
            source.move_until(error, array<const char, 2> { { '*', '/' } });
            return;
        }

        if(*(source.pointer() + 1) == '/') {
            source.move_until(error, '\n', '\r');
            return;
        }
    }
}

/* Parse out ****************************************************/

void PJson::write(const Value& value, const MemBlock& name, Out& out)
{
    auto type = value.type();
    this->insert_spacing(type, out);

    this->scope_closed = false;

    if(this->scope_type != Type::Null &&
       this->scope_type != Type::Array &&
       type != Type::Scope_End) {

        insert_in_quotes(out, name);
        out.write(':');

        if(!this->compact) {
            out.write(' ');
        }
    }

    if(TpTools::is_scope(type)) {

        auto c = type == Type::Array ? '[' : '{';

        out.write(c);
        this->scope_stack.push(type);
        this->scope_closed = true;
        this->scope_type = type;

        return;
    }

    if(type == Type::Scope_End) {

        assert(!this->scope_stack.empty());

        auto c = this->scope_type == Type::Array ? ']' : '}';

        out.write(c);
        this->scope_stack.pop();

        this->scope_type = !this->scope_stack.empty()
            ? this->scope_stack.top()
            : Type::Null;

        return;
    }

    if(TpTools::is_scalar(type)) {
        out.write(value.data(), value.size());

    } else {
        bool escape = type != Type::Binary;
        insert_in_quotes(out, { value.data(), value.size() }, escape);
    }
}

void PJson::insert_spacing(Type type, Out& out)
{
    if(!this->scope_closed && type != Type::Scope_End) {
        out.write(',');
    }
    if(this->compact) {
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

PJson::LitTable::LitTable()
{
    fill_table<0>(this->table);
}

pair<Lib::MemBlock, Value> PJson::read(In& source)
{
    State state;
    while(!state.complete) {

        /* literals can be 'un-quoted', values in 'square-brackets' don't have a name */
        if(!state.reading_value && this->scope_type != Type::Array) {

            source.move_until(error, '\"', ':', '{', '[', '}', ']', '/');

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
            case '/' : skip_comment(source);
                       break;

            default  : this->process_char(source, state, move);
        }

        if(move) {
            source.move(1, error);
        }
    }

    return { state.name, state.value };
}

void PJson::process_bracket(char bracket, State& state)
{
    Type type = bracket == '{' ? Type::Object
              : bracket == '[' ? Type::Array
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

    state.value = Value(type);
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
    if(state.reading_value || this->scope_type == Type::Array) {
        state.value = Value(content, Encoding::UTF8);
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
    bool is_literal = this->lookup.table[*source.pointer()];

    if(is_literal) {

        bool likely_fp = false;
        In::Notify dec ('.', likely_fp);

        auto block = source.read_block_until(error, ',', ' ', '}', ']', dec);

        this->process_literal(block, state, likely_fp ? Type::FP64 : Type::Null);

        if(*source.pointer() == '}' || *source.pointer() == ']') {
            /* The bracket has a double meaning here, first it delimits the literal,
             * and second it closes the scope. So make sure it will be processed. */
            out_move = false;
        }
    }
}

void PJson::process_literal(const MemBlock& str, State& state, Type hint)
{
    bool success; 

    tie(success, state.value) = Tools::str_to_type(str.ptr, str.size, hint);

    if(!success) {
        throw_exception(state, String("Failed to convert string "
                        + String(str).unwrap(false) + " to literal."));
    }

    state.complete = true;
}

void PJson::throw_exception(State& state, const String& info)
{
    auto name = state.name.size > 0
        ? string((const char*)state.name.ptr, state.name.size)
        : "";

    auto msg = "In field " + name + (state.value.data() && state.value.size() > 0
            ? "\' at \'" + string((const char*)state.value.data(), state.value.size()) + "\'."
            : "." );

    throw Exception("Unable to parse JSON document. " + info.unwrap<char>(false) + " " + msg);
}

void PJson::clear()
{
    Aux::clear_stack(this->scope_stack);
    this->scope_type = Type::Null;
    this->scope_closed = true;
}
