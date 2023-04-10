#include "Srl/Srl.h"
#include "Srl/Lib.h"
#include "Srl/TpTools.h"

using namespace std;
using namespace Srl;
using namespace Lib;

namespace {

    const function<void()> error = [] {
        throw Exception("Unable to parse MessagePack data. Data malformed.");
    };

    template<class C>
    C swap_bytes(C in)
    {
        auto N = sizeof(C);

        C out;
        auto* mem_in = (uint8_t*)&in;
        auto* mem_out = (uint8_t*)&out;

        for(auto i = 0U; i < N; i++) {
            mem_out[i] = mem_in[N - i - 1];
        }

        return out;
    }

    /* Positive FixNum 0xxxxxxx 0x00 - 0x7f
       Negative FixNum 111xxxxx 0xe0 - 0xff */
    bool write_pfixnum(uint64_t i, Out& out)
    {
        if(i > 0x7F) {
            return false;
        }

        out.write_byte(i);
        return true;
    }

    bool write_nfixnum(int64_t i, Out& out)
    {
        if(i > -1 || i < -31) {
            return false;
        }

        out.write_byte(0xE0 | (uint8_t)i);
        return true;
    }

    template<Type TP> typename enable_if<TpTools::is_num(TP) && !TpTools::is_fp(TP), void>::type
    write_scalar (const Value& val, Out& out)
    {
        typedef typename TpTools::Real<TP>::type IntType;

        uint8_t lk[] {
            0xD0 /* Type::I8 */,
            0xCC /* Type::UI8 */,
            0xD1 /* Type::I16 */,
            0xCD /* Type::UI16 */,
            0xD2 /* Type::I32 */,
            0xCE /* Type::UI32 */,
            0xD3 /* Type::I64 */,
            0xCF /* Type::UI64 */
        };

        uint8_t prefix = lk[(uint8_t)TP - 1];

        if(TpTools::is_signed(TP)) {

            IntType i = val.pblock().i64;
            if((i >= 0 && write_pfixnum(i, out)) || write_nfixnum(i, out)) {
                return;
            }

            auto swapped = swap_bytes(i);
            out.write(prefix);
            out.write(swapped);

        } else {

            IntType u = val.pblock().ui64;
            if(write_pfixnum(u, out)) {
                return;
            }

            auto swapped = swap_bytes(u);
            out.write(prefix);
            out.write(swapped);
        }
    }

    template<Type TP> typename enable_if<TpTools::is_fp(TP), void>::type
    write_scalar (const Value& val, Out& out)
    {
        if(TP == Type::FP32) {
            out.write_byte(0xCA);

            auto fp = val.pblock().fp32;
            auto swapped = swap_bytes(fp);

            out.write(swapped);

        } else {
            out.write_byte(0xCB);

            auto fp = val.pblock().fp64;
            auto swapped = swap_bytes(fp);

            out.write(swapped);
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

    /* bin  8 11000100 0xc4
       bin 16 11000101 0xc5
       bin 32 11000110 0xc6 */
    void write_bin(const MemBlock& block, Out& out)
    {
        if(block.size <= 255) {
            out.write_byte(0xC4);
            out.write_byte(block.size);

        } else if (block.size <= 0xFFFE) {
            uint16_t sz = block.size;
            out.write_byte(0xC5);
            out.write(sz);

        } else {
            uint32_t sz = block.size;
            out.write_byte(0xC6);
            out.write(sz);
        }

        out.write(block.ptr, block.size);
    }

    /* strfix 101xxxxx 0xa0 - 0xbf
       str  8 11011001 0xd9
       str 16 11011010 0xda
       str 32 11011011 0xdb */
    void write_str(const MemBlock& block, Out& out)
    {
        if(block.size <= 0x1F) {
            out.write_byte(0xA0 | block.size);

        } else if(block.size <= 0xFE) {
            out.write_byte(0xD9);
            out.write_byte(block.size);

        } else if (block.size <= 0xFFFE) {
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

    #define SRL_TYPE_TO_MSGP(id, v, t, s) \
        case Type::id : write_scalar<Type::id>(val, out); break;

    void srl_to_msgp(const Value& val, Out& out)
    {
        switch(val.type()) {

            SRL_TYPES_SCALAR(SRL_TYPE_TO_MSGP)

            case Type::Binary :
                write_bin(MemBlock(val.data(), val.size()), out);
                break;
            case Type::String :
                write_str(MemBlock(val.data(), val.size()), out);
                break;

            default : assert(0);
        }
    }

    #undef SRL_TYPE_TO_MSGP

    bool prefix_is_bin(uint8_t prefix)
    {
        return prefix >= 0xC4 && prefix <= 0xC6;
    }

    bool prefix_is_str(uint8_t prefix)
    {
        return (prefix >= 0xA0 && prefix <= 0xBF) || (prefix >= 0xD9 && prefix <= 0xDB);
    }

    template<Type TP>
    Value read_num(In& in)
    {
        typedef typename TpTools::Real<TP>::type NumTP;

        auto num = in.read_move<NumTP>(error);
        auto swapped = swap_bytes(num);

        return Value(swapped);
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
        return (prefix >= 0xCA && prefix <= 0xD3) || // integers / floats
                prefix <= 0x7F || prefix >= 0xE0  || // pos fixnum / neg fixnum
                prefix == 0xC2 || prefix <= 0xC3  || // true / false
                prefix == 0xC0;                      // null
    }

    Value read_bool(uint8_t prefix)
    {
        bool val = prefix == 0xC3;
        return Value(val);
    }

    Value read_fixnum(uint8_t prefix)
    {
        auto is_neg_fixnum = (1 << 7) & prefix;

        int val;
        if(is_neg_fixnum) {

            uint8_t neg_max = 1 + (0xFF >> 3);
            val = (0xFF >> 3) & prefix;
            val = val - (int)neg_max;

        } else {
            val = (0xFF >> 1) & prefix;
        }

        //if(DEBUG) printf("      read fixnum pfx %02X (%d) -> %d\n", prefix, (is_neg_fixnum), val);
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

    size_t get_str_size(uint8_t prefix, In& in)
    {
        assert(prefix_is_str(prefix));

        return prefix >= 0xA0 && prefix <= 0xBF ? (prefix & 0x1F)
             : prefix == 0xD9 ? in.read_move<uint8_t>(error)
             : prefix == 0xDA ? in.read_move<uint16_t>(error)
             : in.read_move<uint32_t>(error);
    }

    size_t get_bin_size(uint8_t prefix, In& in)
    {
        assert(prefix_is_bin(prefix));

        return prefix == 0xC4 ? in.read_move<uint8_t>(error)
             : prefix == 0xC5 ? in.read_move<uint16_t>(error)
             : in.read_move<uint32_t>(error);
    }

    Value read_str(uint8_t prefix, In& in)
    {
        auto size = get_str_size(prefix, in);
        auto block = in.read_block(size, error);

        return Value(block, Type::String, Encoding::UTF8);
    }

    Value read_bin(uint8_t prefix, In& in)
    {
        auto size = get_bin_size(prefix, in);
        auto block = in.read_block(size, error);

        return Value(block, Type::Binary);
    }

    /* FixMap    1000xxxx 0x80 - 0x8f
       FixArray  1001xxxx 0x90 - 0x9f
       array 16  11011100 0xdc
       array 32  11011101 0xdd
       map 16    11011110 0xde
       map 32    11011111 0xdf */
    bool prefix_is_scope(uint8_t prefix)
    {
        return (prefix >= 0x80 && prefix <= 0x9F) || (prefix >= 0xDC && prefix <= 0xDF);
    }

    pair<Type, size_t> get_scope_info(uint8_t prefix, In& in)
    {
        assert(prefix_is_scope(prefix));

        Type type; size_t size;

        /* big container bit at pos 7 set */
        if(prefix & (1 << 6)) {
            type = prefix & 2 ? Type::Object : Type::Array;
            size = prefix & 1 ? in.read_move<uint32_t>(error) : in.read_move<uint16_t>(error);

        } else {
            type = prefix & (1 << 4) ? Type::Array : Type::Object;
            size = 0xFF >> 4 & prefix;
        }

        return { type, size };
    }

    MemBlock read_name(uint8_t prefix, vector<uint8_t>& buffer, In& in)
    {
        auto val = read_str(prefix, in);
        MemBlock block(val.data(), val.size());

        if(in.is_streaming()) {
            if(buffer.size() < block.size) { buffer.resize(block.size); }

            memcpy(buffer.data(), block.ptr, block.size);
            block = { buffer.data(), block.size };
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
            write_str(name, out);
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
        name = prefix_is_str(prefix) ? read_name(prefix, this->buffer, source) : name;
        prefix = source.read_move<uint8_t>(error);

    }

    if(prefix_is_scope(prefix)) {

        Type type; size_t size;
        tie(type, size) = get_scope_info(prefix, source);

        this->scope_stack.emplace(type, size);
        this->scope = &this->scope_stack.top();

        return { name, type };
    }

    if(prefix_is_str(prefix)) {
        return { name, read_str(prefix, source) };
    }

    if(prefix_is_scalar(prefix)) {
        return { name, read_scalar(prefix, source) };
    }


    if(prefix_is_bin(prefix)) {
        return { name, read_bin(prefix, source) };
    }

    /* shouldn't end here */
    error();
    return { MemBlock(), Type::Null };
}

void PMsgPack::clear()
{
    Aux::clear_stack(this->scope_stack);
    this->scope = nullptr;
}
