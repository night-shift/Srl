#include "Srl/Srl.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

namespace {

    const function<void()> error = [] {
        throw Exception("Unable to parse Srl data. Data malformed.");
    };

    typedef uint8_t Flag;

    const Flag FNum     = 1;

    const Flag FNamed   = 1 << 1;
    const Flag FIndexed = 1 << 2; /* !set -> new */

    /* num set */
    const Flag FSigned = 1 << 3; /* !set -> unsigned */
    const Flag FFP32   = 1 << 4;
    const Flag FFP64   = 1 << 5;
    const Flag FTrue   = 1 << 6;
    const Flag FFalse  = 1 << 7;

    /* !num set */
    const Flag FNull   = 1 << 3;
    const Flag FBinary = 1 << 4;
    const Flag FString = 1 << 5;
    const Flag FObject = 1 << 6;
    const Flag FArray  = 1 << 7;
    /* no flags set -> scope end */

    bool is_scope    (Flag flag) { return !(flag & FNum) && flag & (FObject | FArray); }

    Flag build_flag(const Value& val)
    {
        auto tp = val.type();

        if(tp == Type::Null) {
            return FNull;
        }

        if(TpTools::is_scope(tp)) {
            return tp == Type::Array ? FArray : FObject;
        }

        if(tp == Type::String || tp == Type::Binary) {
            return FBinary;
        }

        if(TpTools::is_fp(tp)) {
            return FNum | (tp == Type::FP32 ? FFP32 : FFP64);
        }

        if(tp == Type::Bool) {
            return FNum | (val.pblock().i64 ? FTrue : FFalse);
        }

        if(TpTools::is_integral(tp)) {
            return FNum | (val.pblock().i64 < 0 ? FSigned : 0);
        }

        return 0;
    }

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

/* Check if a particular field id was written already.
 * If yes, write StringPrefix::Indexed flag followed by the n-th position(encoded) of that string.
 * If no, associate the string with the n-th occurrence of a new string,
 * write StringPrefix::New followed by the length of that string and the string itself.
 *
 * This saves a few bytes on multiple occurrences of the same data-structure almost for free.
 * Worst case you get an unnecessary extra byte per field index.
 * */
void PSrl::write_flag(Flag flag, const MemBlock& str, Out& out)
{
    if(str.size < 1 || scope == Type::Null || scope == Type::Array) {
        out.write_byte(flag);
        return;
    }

    flag |= FNamed;

    Tools::HashFnv1a<MemBlock> hash_function;

    auto hash = hash_function(str);
    auto itr  = this->hashed_strings.find(hash);

    if(itr != this->hashed_strings.end()) {
        auto found_index = itr->second;

        out.write_byte(flag | FIndexed);
        encode_integer(found_index, out);

    } else {
        out.write_byte(flag);

        auto index = this->n_hashed_strings++;
        this->hashed_strings.insert({ hash, index });

        encode_integer(str.size, out);
        out.write(str);
    }
}
/* the above in reverse */
pair<Flag, MemBlock> PSrl::read_flag(In& source)
{
    auto flag = source.cast_move<uint8_t>(error);

    if(!(flag & FNamed) || scope == Type::Null || scope == Type::Array) {
        return { flag, MemBlock() };
    }

    if(flag & FIndexed) {
        auto index = decode_integer(source);
        if(index >= this->indexed_strings.size()) {
            error();
        }

        return { flag, this->indexed_strings[index] };

    } else {
        auto size  = decode_integer(source);
        auto block = source.read_block(size, error);

        if(source.is_streaming()) {
            block = copy_block(this->string_buffer, block);
        }

        auto& str = *this->indexed_strings.insert(indexed_strings.end(), block);

        return { flag, str };
    }
}

void PSrl::parse_out(const Value& value, const MemBlock& name, Out& out)
{
    auto flag = build_flag(value);

    if(!flag) {
        out.write_byte(0);

        assert(this->scope_stack.size() > 0);
        this->pop_scope();
        return;
    }

    this->write_flag(flag, name, out);
    
    auto type = value.type();
    /* scope-starts carry no additional information */
    if(TpTools::is_scope(type)) {
        this->push_scope(type);
        return;
    }

    /* do nothing on null-type, flag already carries bool value */
    if(type == Type::Null | type == Type::Bool) {
        return;
    }

    if(type == Type::FP32) {
        out.write(value.pblock().fp32);

    } else if(type == Type::FP64) {
        out.write(value.pblock().fp64);

    } else if(TpTools::is_integral(type)) {

        uint64_t integer = TpTools::is_signed(type) && value.pblock().i64 < 0
            ? -value.pblock().i64
            : value.pblock().ui64;

        encode_integer(integer, out);

    } else {
        assert(type == Type::Binary || type == Type::String);

        encode_integer(value.size(), out);
        out.write(value.data(), value.size());
    }
}

Parser::SourceSeg PSrl::parse_in(In& source)
{
    Flag flag; MemBlock name;
    tie(flag, name) = this->read_flag(source);

    if(!flag) {
        if(this->scope_stack.size() < 1) {
            error();
        }
        this->pop_scope();
        return SourceSeg(Type::Scope_End);
    }

    if(is_scope(flag)) {
        auto tp = flag & FObject ? Type::Object : Type::Array;
        this->push_scope(tp);

        return SourceSeg(Value(tp), name);
    }

    if(flag & FNum) {
        return this->read_num(flag, name, source);
    }


    if(flag & FBinary) {
        auto size  = decode_integer(source);
        auto block = source.read_block(size, error);

        return SourceSeg(Value(block, Type::Binary, Encoding::UTF8), name);
    }

    if(flag & FNull) {
        return SourceSeg(Value(Type::Null), name);
    }

    /* shouldn't end down here */
    error();
    return SourceSeg(Type::Null);
}

Parser::SourceSeg PSrl::read_num(Flag flag, const MemBlock& name, In& source)
{
    if(flag & (FTrue | FFalse)) {
        bool val = flag & FTrue;
        return { Value(val), name };
    }

    if(flag & FFP32) {
        float val = source.cast_move<float>(error);
        return { Value(val), name };
    }

    if(flag & FFP64) {
        double val = source.cast_move<double>(error);
        return { Value(val), name };
    }

    uint64_t integer = decode_integer(source);

    return flag & FSigned
        ? SourceSeg(Value(-(int64_t)integer), name)
        : SourceSeg(Value(integer), name);
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
