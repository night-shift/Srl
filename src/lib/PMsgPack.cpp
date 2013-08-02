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
    bool write_pfixnum(uint64_t i, Out& out)
    {
        if(i <= 0x7F) {
            out.write_byte(i);
            return true;
        }
        return false;
    }

    bool write_nfixnum(int64_t i, Out& out)
    {
        if(i < 0 && i >= -31) {
            out.write_byte(0xE0 | (uint8_t)-i);
            return true;
        } 
        return false;
    }

    template<Type TP> typename enable_if<TpTools::is_num(TP) && !TpTools::is_fp(TP), void>::type
    write_scalar (const Value& val, Out& out)
    {
        typedef typename TpTools::Real<TP>::type Real;

        const uint8_t lk[] {
            0xD0 /* Type::I8 */,
            0xCC /* Type::UI8 */,
            0xD1 /* Type::I16 */,
            0xCD /* Type::UI16 */,
            0xD2 /* Type::I32 */,
            0xCE /* Type::UI32 */,
            0xD3 /* Type::I64 */,
            0xCF /* Type::UI64 */
        };

        const uint8_t prefix = lk[(uint8_t)TP - 1];

        if(TpTools::is_signed(TP)) {

            int64_t i = val.pblock().i64;
            if((i >= 0 && write_pfixnum(i, out)) || write_nfixnum(i, out)) {
                return;
            }

            out.write(prefix); 
            out.write((Real)i);

        } else {

            uint64_t i = val.pblock().ui64;
            if(write_pfixnum(i, out)) {
                return;
            }

            out.write(prefix); 
            out.write((Real)i);
        }
    }

    template<Type TP> typename enable_if<TpTools::is_fp(TP), void>::type
    write_scalar (const Value& val, Out& out)
    {
        if(TP == Type::FP32) {
            out.write_byte(0xCA); 
            out.write(val.pblock().fp32);

        } else {
            out.write_byte(0xCB); 
            out.write(val.pblock().fp64);
        }
    }

    template<Type TP> typename enable_if<TP == Type::Bool, void>::type
    write_scalar (const Value& val, Out& out)
    {
        out.write_byte(val.pblock().ui64 ? 0xC3 : 0xC2); 
    }

    template<Type TP> typename enable_if<TP == Type::Null, void>::type
    write_scalar (const Value&, Out& out)
    {
        out.write_byte(0xC0); 
    }

    /* FixRaw 101xxxxx 0xa0 - 0xbf
       raw 16 11011010 0xda
       raw 32 11011011 0xdb */
    void write_raw(const MemBlock& block, Out& out)
    {
        if(block.size <= 16) {
            out.write_byte(0xA0 | block.size);

        } else if (block.size <= 0xFFFF) {
            uint16_t sz = block.size;
            out.write_byte(0xDA);
            out.write(sz);

        } else {
            uint32_t sz = block.size;
            out.write_byte(0xDB);
            out.write(sz);
        }

        out.write(block.ptr, block.size);
    }
    
    void write_raw(const Value& val, Out& out)
    {
        write_raw(MemBlock(val.data(), val.size()), out);
    }

    #define SRL_TYPE_TO_MSGP(id, v, t, s) \
        case Type::id : write_scalar<Type::id>(value, out); break;

    void srl_to_msgp(const Value& value, Out& out)
    {
        switch(value.type()) {

            SRL_TYPES_SCALAR(SRL_TYPE_TO_MSGP)

            case Type::Binary :
            case Type::String : write_raw(value, out); break;

            default : assert(0);
        }
    }

    #undef SRL_TYPE_TO_MSGP

    bool prefix_is_raw(uint8_t prefix)
    {
        return (prefix >= 0xA0 && prefix <= 0xBF) || prefix == 0xDA || prefix == 0xDB; 
    }

    size_t get_raw_size(uint8_t prefix, In& in)
    {
        assert(prefix_is_raw(prefix));

        return prefix <= 0xBF ? prefix & 0xFF >> 3
             : prefix & 1 ? in.read_move<uint32_t>(error)
             : in.read_move<uint16_t>(error);
    }

    template<Type TP> Value read_num(In& in)
    {
        auto val = in.read_move<typename TpTools::Real<TP>::type>(error);
        return Value(val);
    }

    Value read_num(uint8_t prefix, In& in)
    {
        switch(prefix) {
            case 0xCA : return read_num<Type::FP32>(in);
            case 0xCB : return read_num<Type::FP64>(in);
            case 0xCC : return read_num<Type::UI8>(in);
            case 0xCD : return read_num<Type::UI16>(in);
            case 0xCE : return read_num<Type::UI32>(in);
            case 0xCF : return read_num<Type::UI64>(in);
            case 0xD0 : return read_num<Type::I8>(in);
            case 0xD1 : return read_num<Type::I16>(in);
            case 0xD2 : return read_num<Type::I32>(in);
            case 0xD3 : return read_num<Type::I64>(in);

            default : error(); return Value(Type::Null);
        };
    }

    bool prefix_is_scalar(uint8_t prefix)
    {
        return (prefix >= 0xCA && prefix <= 0xD3) ||
                prefix <= 0x7F || prefix >= 0xE0  ||
                prefix == 0xC2 || prefix == 0xC3  ||
                prefix == 0xC0;
    }

    Value read_bool(uint8_t prefix)
    {
        bool val = prefix == 0xC3;
        return Value(val);
    }

    Value read_fixnum(uint8_t prefix)
    {
        int val;
        if(1 << 7 & prefix) {
            val = 0xFF >> 3 & prefix;
            val = -val;

        } else {
            val = 0x7F & prefix;
        }

        return Value(val);
    }

    Value read_scalar(uint8_t prefix, In& in)
    {
        assert(prefix_is_scalar(prefix));

        return prefix == 0xC2 || prefix == 0xC3 ? read_bool(prefix) 
             : prefix >= 0xCA && prefix <= 0xD3 ? read_num(prefix, in) 
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
           size = prefix & 0x01 ? in.read_move<uint32_t>(error) : in.read_move<uint16_t>(error);

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

void PMsgPack::write(const Value& value, const MemBlock& name, Out& out)
{
    auto type = value.type();

    if(type == Type::Scope_End) {
        assert(this->scope_stack.size() > 0);

        out.write(this->scope->ticket, (const uint8_t*)&scope->elements, 0, 4);
        this->scope_stack.pop();
        this->scope = this->scope_stack.size() > 0 ? &this->scope_stack.top() : nullptr;

        return;
    }

    if(this->scope) { 
        if(this->scope->type == Type::Object) {
            write_raw(name, out);
        }
        this->scope_stack.top().elements++;
    }


    if(TpTools::is_scope(type)) {
        this->write_scope(type, out);
        return;
    }

    srl_to_msgp(value, out);
}

void PMsgPack::write_scope(Type type, Out& out)
{
    uint8_t prefix = type == Type::Object ? 0xDF : 0xDD;

   out.write(prefix); 
   auto ticket = out.reserve(4);
   this->scope_stack.emplace(type, 0, ticket);
   this->scope = &this->scope_stack.top();
}

pair<MemBlock, Value> PMsgPack::read(In& source)
{
    if(this->scope) {

        if(this->scope->elements < 1) {
            if(this->scope_stack.size() < 1) {
                error();
            }
            this->scope_stack.pop();
            this->scope = this->scope_stack.size() > 0 ? &this->scope_stack.top() : nullptr;

            return { MemBlock(), Type::Scope_End };
        }

        this->scope->elements--;
    }

    MemBlock name;
    auto prefix = source.read_move<uint8_t>(error);

    if(this->scope && this->scope->type == Type::Object) {
        name = prefix_is_raw(prefix) ? read_name(prefix, this->buffer, source) : name;
        prefix = source.read_move<uint8_t>(error);
    }

    if(prefix_is_scope(prefix)) {
        Type type; size_t size;
        tie(type, size) = get_scope_info(prefix, source);

        this->scope_stack.emplace(type, size);
        this->scope = &this->scope_stack.top();

        return { name, type }; 
    }

    if(prefix_is_scalar(prefix)) {
        return { name, read_scalar(prefix, source) };
    }

    if(prefix_is_raw(prefix)) {
        return { name, read_raw(prefix, source) };
    }

    /* shouldn't end here */
    error();
    return { MemBlock(), Type::Null };
}
