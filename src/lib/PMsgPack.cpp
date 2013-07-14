#include "Srl/Srl.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

namespace {

    const function<void()> error = [] {
        throw Exception("Unable to parse MessagePack data. Data malformed.");
    };

    /* Positive FixNum 0xxxxxxx 0x00 - 0x7f
       Negative FixNum 111xxxxx 0xe0 - 0xff */
    template<uint8_t prefix, Type type>
    function<void(const MemBlock&, Out& out)> scalar_to_msgp()
    {
        return [](const MemBlock& val, Out& out) {

            const auto write_pfixnum = [&out](uint64_t i) {
                if(i <= 0x7F) {
                    out.write((uint8_t)i);
                    return true;
                }
                return false;

            };

            if(TpTools::is_signed(type)) {
                int64_t i;
                TpTools::paste_type(i, type, val.ptr);

                if(i < 0 && i >= -31) {
                    out.write(0xE0 | (uint8_t)-i);
                    return;

                } else if(i >= 0 && write_pfixnum(i)) { 
                    return;
                }

            } else {

                uint64_t i;
                TpTools::paste_type(i, type, val.ptr);
                if(write_pfixnum(i)) {
                    return;
                }
            }

            out.write(prefix); 
            out.write(val.ptr, val.size);
        };
    }

    template<uint8_t prefix> function<void(const MemBlock&, Out& out)> fp_to_msgp()
    {
        return [](const MemBlock& val, Out& out) {
            out.write(prefix); 
            out.write(val.ptr, val.size);
        };
    }

    /* FixRaw 101xxxxx 0xa0 - 0xbf
       raw 16 11011010 0xda
       raw 32 11011011 0xdb */
    const auto raw_to_msgp = [](const MemBlock& val, Out& out) {
        if(val.size < 16) {
            out.write(0xA0 | (uint8_t)val.size);

        } else if (val.size <= (uint16_t)~0U) {
            uint16_t sz = val.size;
            out.write(0xDA);
            out.write((const uint8_t*)&sz, 2);

        } else {
            uint32_t sz = val.size;
            out.write(0xDB);
            out.write((const uint8_t*)&sz, 4);
        }

        out.write(val.ptr, val.size);
    };

    bool prefix_is_raw(uint8_t prefix)
    {
        return (prefix >= 0xA0 && prefix <= 0xBF) || prefix == 0xDA || prefix == 0xDB; 
    }

    size_t get_raw_size(uint8_t prefix, In& in)
    {
        assert(prefix_is_raw(prefix));

        return prefix <= 0xBF ? prefix & 0xFF >> 3
             : prefix & 1  ? in.cast_move<uint32_t>(error)
             : in.cast_move<uint16_t>(error);
    }

    const function<void(const MemBlock&, Out& out)> srl_to_msgp[] {
        /* false 0xc2 true 0xc3 */
        [](const MemBlock& val, Out& out) { uint8_t t = *val.ptr ? 0xC3 : 0xC2; out.write(t); },
        scalar_to_msgp<0xD0, Type::I8>(),
        scalar_to_msgp<0xCC, Type::UI8>(),
        scalar_to_msgp<0xD1, Type::I16>(),
        scalar_to_msgp<0xCD, Type::UI16>(),
        scalar_to_msgp<0xD2, Type::I32>(),
        scalar_to_msgp<0xCE, Type::UI32>(),
        scalar_to_msgp<0xD3, Type::I64>(),
        scalar_to_msgp<0xCF, Type::UI64>(),
        fp_to_msgp<0xCA>(),
        fp_to_msgp<0xCB>(),
        /* nil 0xc0 Type::Null*/
        [](const MemBlock&, Out& out) { out.write(0xC0); },
        /* Type::String*/
        raw_to_msgp,
        /* Type::Binary*/
        raw_to_msgp,
    };

    template<Type type> function<Value(In& in)> integral_to_srl()
    {
        return [](In& in) {
            auto sz = TpTools::get_size(type);
            auto block = in.read_block(sz, error);
            return Value(block, type);
        };
    }

    const function<Value(In& in)> msgp_to_srl[] {
    /* float   0xca */ integral_to_srl<Type::FP32>(),
    /* double  0xcb */ integral_to_srl<Type::FP64>(),
    /* uint 8  0xcc */ integral_to_srl<Type::UI8>(),
    /* uint 16 0xcd */ integral_to_srl<Type::UI16>(),
    /* uint 32 0xce */ integral_to_srl<Type::UI32>(),
    /* uint 64 0xcf */ integral_to_srl<Type::UI64>(),
    /* int 8   0xd0 */ integral_to_srl<Type::I8>(),
    /* int 16  0xd1 */ integral_to_srl<Type::I16>(),
    /* int 32  0xd2 */ integral_to_srl<Type::I32>(),
    /* int 64  0xd3 */ integral_to_srl<Type::I64>()
    };

    bool prefix_is_integral(uint8_t prefix)
    {
        return (prefix >= 0xCA && prefix <= 0xD3) ||
                prefix <= 0x7F || prefix >= 0xE0  ||
                prefix == 0xC2 || prefix == 0xC3  ||
                prefix == 0xC0;
    }

    Value read_bool(uint8_t prefix)
    {
        uint8_t val = prefix == 0xC3 ? 1 : 0;
        return Value::From_Type({ &val, 1 }, Type::Bool);
    }

    Value read_fixnum(uint8_t prefix)
    {
        Type type; uint8_t val;

        if(1 << 7 & prefix) {
            val = 0xFF >> 3 & prefix;
            val = -val;
            type = Type::I8;

        } else {
            val = 0x7F & prefix;
            type = Type::UI8;
        }
        return Value::From_Type({ &val, 1 }, type);
    }

    Value read_integral(uint8_t prefix, In& in)
    {
        assert(prefix_is_integral(prefix));

        return prefix == 0xC2 || prefix == 0xC3 ? read_bool(prefix) 
             : prefix >= 0xCA && prefix <= 0xD3 ? msgp_to_srl[prefix - 0xCA](in) 
             : prefix == 0xC0 ? Value(MemBlock(), Type::Null)
             : read_fixnum(prefix);
    }

    Value read_raw(uint8_t prefix, In& in)
    {
        auto size = get_raw_size(prefix, in);
        auto block = in.read_block(size, error);

        return Value(block, Type::Binary, Encoding::UTF8);
    }

    /* FixMap	 1000xxxx 0x80 - 0x8f
       FixArray	 1001xxxx 0x90 - 0x9f
       array 16	 11011100 0xdc
       array 32	 11011101 0xdd
       map 16	 11011110 0xde
       map 32	 11011111 0xdf */

    bool prefix_is_scope(uint8_t prefix)
    {
        return (prefix >= 0x80 && prefix <= 0x9F) || (prefix >= 0xDC && prefix <= 0xDF);
    }

    pair<Type, size_t> get_scope_info(uint8_t prefix, In& in)
    {
        assert(prefix_is_scope(prefix));

        Type type; size_t size;

        if(0x0D << 4 & prefix) {
           type = prefix <= 0xDD ? Type::Array : Type::Object;
           size = prefix & 0x01 ? in.cast_move<uint32_t>(error) : in.cast_move<uint16_t>(error);

        } else {
            assert(1 << 7 & prefix);

            type = prefix & 1 << 4 ? Type::Array : Type::Object;
            size = 0xFF >> 4 & prefix;
        }

        return { type, size };
    }

    MemBlock read_name(uint8_t prefix, vector<uint8_t>& buffer, In& in)
    {
        auto size = get_raw_size(prefix, in);
        auto block = in.read_block(size, error);

        if(in.is_streaming()) {
            if(buffer.size() < size) { buffer.resize(size); }

            memcpy(buffer.data(), block.ptr, size);
            block = { buffer.data(), size };
        }

        return block;
    }
}

void PMsgPack::parse_out(const Value& value, const MemBlock& name, Out& out)
{
    auto type = value.type();

    if(type == Type::Scope_End) {
        assert(this->scope_stack.size() > 0);

        out.write(this->scope->ticket, (const uint8_t*)&scope->elements, 0, 4);
        this->scope_stack.pop();
        this->scope = this->scope_stack.size() > 0 ? &this->scope_stack.top() : nullptr;

        return;
    }

    if(this->scope != nullptr) { 
        if(this->scope->type == Type::Object) {
            srl_to_msgp[(uint8_t)Type::String](name, out);
        }

        this->scope_stack.top().elements++;
    }


    if(TpTools::is_scope(type)) {
        this->write_scope(type, out);
        return;
    }

    srl_to_msgp[(uint8_t)type]({ value.data(), value.size() }, out);
}

void PMsgPack::write_scope(Type type, Out& out)
{
    uint8_t prefix = type == Type::Object ? 0xDF : 0xDD;

   out.write(prefix); 
   auto ticket = out.reserve(4);
   this->scope_stack.emplace(type, 0, ticket);
   this->scope = &this->scope_stack.top();
}

Parser::SourceSeg PMsgPack::parse_in(In& source)
{
    if(this->scope != nullptr) {

        if(this->scope->elements < 1) {
            if(this->scope_stack.size() < 1) {
                error();
            }
            this->scope_stack.pop();
            this->scope = this->scope_stack.size() > 0 ? &this->scope_stack.top() : nullptr;

            return SourceSeg(Type::Scope_End);
        }

        this->scope->elements--;
    }

    MemBlock name;
    auto prefix = source.cast_move<uint8_t>(error);

    if(this->scope != nullptr && this->scope->type == Type::Object) {
        name = prefix_is_raw(prefix) ? read_name(prefix, this->buffer, source) : name;
        prefix = source.cast_move<uint8_t>(error);
    }

    if(prefix_is_scope(prefix)) {
        Type type; size_t size;
        tie(type, size) = get_scope_info(prefix, source);

        this->scope_stack.emplace(type, size);
        this->scope = &this->scope_stack.top();

        return SourceSeg(type, name); 
    }

    if(prefix_is_integral(prefix)) {
        return { read_integral(prefix, source), name, source.is_streaming() };
    }

    if(prefix_is_raw(prefix)) {
        return { read_raw(prefix, source), name, source.is_streaming() };
    }

    /* shouldn't end here */
    error();
    return SourceSeg(Type::Null);
}
