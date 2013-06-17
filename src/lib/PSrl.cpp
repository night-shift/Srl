#include "Srl/Srl.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

namespace {

    const function<void()> error = [] {
        throw Exception("Unable to parse Srl data. Data malformed.");
    };

    /* variable-length quantity encoding */
    void     encode_integer(uint64_t integer, Out& buffer);
    uint64_t decode_integer(In& source);
}

/* Check if a particular field id was written already.
 * If yes, write StringPrefix::Indexed flag followed by the n-th position(encoded) of that string.
 * If no, associate the string with the n-th occurrence of a new string,
 * write StringPrefix::New followed by the length of that string and the string itself.
 *
 * This saves a few bytes on multiple occurrences of the same data-structure almost for free.
 * Worst case you get an unnecessary extra byte per field index.
 * */
void PSrl::write_string(const MemBlock& str, Out& out)
{
    if(str.size < 1) {
        out.write((uint8_t)StringPrefix::Empty);
        return;
    }

    Tools::HashFnv1a<MemBlock> hash_function;

    auto hash = hash_function(str);
    auto itr  = this->hashed_strings.find(hash);

    if(itr != this->hashed_strings.end()) {
        auto found_index = itr->second;
        out.write((uint8_t)StringPrefix::Indexed);
        encode_integer(found_index, out);

    } else {
        out.write((uint8_t)StringPrefix::New);
        auto index = this->n_hashed_strings++;

        this->hashed_strings.insert({ hash, index });
        encode_integer(str.size, out);
        out.write(str);
    }
}
/* the above in reverse */
MemBlock PSrl::read_string(In& source)
{
    auto prefix_token = source.cast_move<uint8_t>(error);
    if(!this->valid_string_prefix(prefix_token)) {
        error();
    }
    auto prefix = (StringPrefix)prefix_token;
    if(prefix == StringPrefix::Empty) {
        return MemBlock();
    }

    if(prefix == StringPrefix::Indexed) {
        auto index = decode_integer(source);
        if(index >= this->indexed_strings.size()) {
            error();
        }
        return this->indexed_strings[index];

    } else {
        assert(prefix == StringPrefix::New && "Unknown string prefix.");

        auto size  = decode_integer(source);
        auto block = source.read_block(size, error);

        if(source.is_streaming()) {
            block = copy_block(this->string_buffer, block);
        }

        return *this->indexed_strings.insert(indexed_strings.end(), block);
    }
}

void PSrl::parse_out(const Value& value, const MemBlock& name, Out& out)
{
    auto type = value.type();
    /* write the value type */
    out.write((uint8_t)type);
    if(type == Type::Scope_End) {

        assert(this->scope_stack.size() > 0);
        this->pop_scope();
        return;
    }

    if(this->scope == Type::Null || this->scope != Type::Array) {
        /* write field name */
        this->write_string(name, out);
    }
    /* scope-starts carry no additional information */
    if(TpTools::is_scope(type)) {
        this->push_scope(type);
        return;
    }

    if(TpTools::is_literal(type)) {
        /* do nothing on null-type, encode on integral-type */
        if(type == Type::Null) {
            return;
        }

        if(type == Type::FP32 || type == Type::FP64) {
            out.write(value.data(), value.size());

        } else {
            uint64_t integer = 0;
            assert(value.size() < 9 && "Cannot encode integers > 64 bit.");
            /* TODO support for big-endian */
            memcpy(&integer, value.data(), value.size());
            encode_integer(integer, out);
        }

    } else {
        assert(type == Type::Binary || type == Type::String);

        encode_integer(value.size(), out);
        out.write(value.data(), value.size());
    }
}

Parser::SourceSeg PSrl::parse_in(In& source)
{
    auto type_token = source.cast_move<uint8_t>(error);
    if(!TpTools::is_valid(type_token)) {
        error();
    }
    auto type = (Type)type_token;

    if(type == Type::Scope_End) {
        if(this->scope_stack.size() < 1) {
            error();
        }
        this->pop_scope();
        return SourceSeg(Type::Scope_End);
    }

    auto name = this->scope == Type::Null || this->scope != Type::Array
        ? this->read_string(source)
        : MemBlock();

    if(TpTools::is_scope(type)) {
        this->push_scope(type);

        return SourceSeg(Value(type), name);
    }

    if(TpTools::is_literal(type)) {
        return this->read_literal(name, source, type);
    }

    if(type == Type::String || type == Type::Binary) {
        auto size  = decode_integer(source);
        auto block = source.read_block(size, error);

        bool buffered = source.is_streaming();

        return type == Type::String ? SourceSeg({ block, Encoding::UTF8 }, name, buffered)
                                    : SourceSeg({ block, Type::Binary }, name, buffered);
    }

    /* shouldn't end down here */
    error();
    return SourceSeg(Type::Null);
}

Parser::SourceSeg PSrl::read_literal(const MemBlock& name, In& source, Type type)
{
    if(type == Type::Null) {
        return { Value({ }, type), name, false };
    }

    if(type == Type::FP32 || type == Type::FP64) {
        auto type_size = TpTools::get_size(type);
        auto block = source.read_block(type_size, error);

        return { Value(block, type), name, source.is_streaming() };
    }

    auto size        = TpTools::get_size(type);
    uint64_t integer = decode_integer(source);
    /* TODO support for big-endian */
    memcpy(this->integer_buffer, &integer, size);

    return { Value({ this->integer_buffer, size }, type), name, true };
}

void PSrl::push_scope(Type scope_type)
{
    this->scope_stack.push(scope_type);
    this->scope = scope_type;
}

void PSrl::pop_scope()
{
    this->scope_stack.pop();
    this->scope = this->scope_stack.size() > 0
        ? this->scope_stack.top()
        : Type::Null;
}

namespace {

    const uint64_t mask_msb       = 128U; /* 0...1000 0000 msb in a block, tells us more blocks are comming */
    const uint64_t mask_reset_msb = 127U; /* 0...0111 1111 */

    void encode_integer(uint64_t integer, Out& out)
    {
        uint8_t buffer[10];
        auto sz = 0U;

        while(integer >= mask_msb) {
            auto block = (integer | mask_msb);
            integer >>= 7;
            buffer[sz++] = (uint8_t)block;
        }

        assert(sz < 10);

        /* just the lowest 7 bits left */
        buffer[sz++] = (uint8_t)integer;
        out.write(buffer, sz);
    }

    uint64_t decode_integer(In& source)
    {
        uint8_t  block       = *source.pointer();
        uint64_t out_integer = mask_reset_msb & block;

        auto shift = 7U;
        /* first block w/o msb set is the last 8-bit block of the integer to decode */
        while(mask_msb & block) {
            /* no support for integers > 64 bit */
            if(shift > 9 * 7) {
                error();
            }
            source.move(1, error);
            block = *source.pointer();
            out_integer |= (mask_reset_msb & block) << shift;
            shift += 7;
        }

        source.move(1, error);

        return out_integer;
    }
}
