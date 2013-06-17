#include "Srl/Srl.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

namespace {

    const function<void()> error = [] {
        throw Exception("Unable to parse BSON data. Data malformed.");
    };

    /* straight mapping from the bson-specification, see: http://bsonspec.org/#/specification */

    const uint8_t Bson_Scope_End       = 0x00;
    const uint8_t Bson_Scope_Object    = 0x03;
    const uint8_t Bson_Scope_Container = 0x04;
    const uint8_t Bson_String          = 0x02;
    const uint8_t Bson_Binary          = 0x05;
    const uint8_t Bson_Sub_binary      = 0x00;

    const map<Type, uint8_t> srl_to_bson {
        /*  0x10 -> Int32 / 0x12 -> Int64 */
        { Type::I8,        0x10 },
        { Type::UI8,       0x10 },
        { Type::I16,       0x10 },
        { Type::UI16,      0x10 },
        { Type::I32,       0x10 },
        { Type::UI32,      0x10 },
        { Type::I64,       0x12 },
        { Type::UI64,      0x12 },
        { Type::FP32,      0x01 },
        { Type::FP64,      0x01 },
        { Type::Bool,      0x08 }, /* 0x08 [name] 0x00 -> false / 0x01 -> true */
        { Type::Null,      0x0A },
        { Type::Object,    0x03 },
        { Type::Array,     0x04 },
        { Type::String,    0x02 },
        { Type::Binary,    0x05 },
        { Type::Scope_End, 0x09 }
    };

    const Type bson_to_srl[] = {
        /*document end       0x00 */ Type::Scope_End,
        /*double             0x01 */ Type::FP64,
        /*string             0x02 */ Type::String,
        /*document           0x03 */ Type::Object,
        /*array              0x04 */ Type::Array,
        /*binary             0x05 */ Type::Binary,
        /*undefined          0x06 */ Type::Null,
        /*Object-id          0x07 */ Type::Null, /* not supported */
        /*bool               0x08 */ Type::Bool,
        /*int64 UTC          0x09 */ Type::I64,
        /*null value         0x0A */ Type::Null,
        /*regular expression 0x0B */ Type::String,
        /*string deprecated  0x0C */ Type::String,
        /*string javascript  0x0D */ Type::String,
        /*string deprecated  0x0E */ Type::String,
        /*code_w_s           0x0F */ Type::String,
        /*int32              0x10 */ Type::I32,
        /*int64 time-stamp   0x11 */ Type::I64,
        /*int64              0x12 */ Type::I64,

        /*min key    0xFF not supported */
        /*max key    0x7F not supported */
    };

    bool is_string_prefix(uint8_t bson_prefix)
    {
        return bson_prefix == 0x02 || bson_prefix == 0x0B ||
               bson_prefix == 0x0D || bson_prefix == 0x0F;
    }

    Value read_binary(In& source);
    Value read_string(In& source);
    Value read_value(In& source, uint8_t bson_prefix);
}

/* Parse out ****************************************************/

void PBson::parse_out(const Value& value, const MemBlock& name, Out& out)
{
    if(TpTools::is_scope(value.type())) {
        auto prefix = value.type() == Type::Array ? Bson_Scope_Container : Bson_Scope_Object;
        if(this->scope_stack.size() > 0) {
            this->insert_prefix(prefix, name, out);
        }
        this->insert_scope(value.type(), out);

        return;
    }

    if(value.type() == Type::Scope_End) {
        this->insert(&Bson_Scope_End, 1, out);
        assert(!this->scope_stack.empty());

        /* write document size */
        auto& closing_scope = this->scope_stack.top();
        uint32_t doc_sz = closing_scope.document_size;
        auto& ticket = closing_scope.ticket;

        out.write(ticket, (const uint8_t*)&doc_sz, 0, 4);

        this->scope_stack.pop();
        /* update previous document size */
        if(this->scope_stack.size() > 0) {
            this->scope_stack.top().document_size += doc_sz;
        }

        return;
    }

    auto prefix = srl_to_bson.at(value.type());
    auto type   = value.type();

    MemBlock mem_block(value.data(), value.size());

    this->insert_prefix(prefix, name, out);

    if(type == Type::Binary) {
        this->insert_binary(mem_block, out);
    }
    else if(type == Type::String) {
        this->insert_string(mem_block, out);
    }
    else {
        assert(TpTools::is_literal(type));
        this->insert_literal(mem_block, type, out);
    }
}

void PBson::insert(const uint8_t* data, size_t size, Out& out)
{
    assert(this->scope_stack.size() > 0);

    this->scope_stack.top().document_size += size;
    out.write(data, size);
}

void PBson::insert_prefix(uint8_t prefix, const MemBlock& name, Out& out)
{
    this->insert(&prefix, 1, out);
    /* Bson array elements are string-indexed */
    if(this->scope_stack.size() > 0 && this->scope_stack.top().type == Type::Array) {

        uint32_t n_elems = this->scope_stack.top().elements++;
        auto str_len = Tools::type_to_string(Type::UI32, (const uint8_t*)&n_elems, this->buffer);

        if(this->buffer.size() < str_len + 1) {
            this->buffer.resize(str_len + 1);
        }

        this->buffer[str_len] = '\0';
        this->insert(&this->buffer[0], str_len + 1, out);

    } else {
        this->insert(name.ptr, name.size, out);
        out.write('\0');
        this->scope_stack.top().document_size++;
    }
}

void PBson::insert_scope(Type type, Out& out)
{
    /* int32 document size */
    auto ticket = out.reserve(4);
    this->scope_stack.emplace(type, ticket);
    this->scope_stack.top().document_size += 4;
}

void PBson::insert_literal(MemBlock& block, Type type, Out& out)
{
    int32_t val_int = 0;
    double  val_double = 0.0;
    uint8_t val_bool = 0;

    switch (type) {
        /* bson spec does not have either a 32 bit floating or a sub 32 bit integer type */
        case Type::I8   :
        case Type::UI8  : val_int = type == Type::UI8
                            ? Read_Cast<uint8_t>(block.ptr)
                            : Read_Cast<int8_t>(block.ptr);
                          this->insert((const uint8_t*)&val_int, 4, out);
                          break;
        case Type::I16  :
        case Type::UI16 : val_int = type == Type::UI16
                            ? Read_Cast<uint16_t>(block.ptr)
                            : Read_Cast<int16_t>(block.ptr);
                          this->insert((const uint8_t*)&val_int, 4, out);
                          break;
        case Type::I32  :
        case Type::UI32 : this->insert(block.ptr, 4, out);
                          break;

        case Type::FP32 : val_double = Read_Cast<float>(block.ptr);
                          this->insert((const uint8_t*)&val_double, 8, out);
                          break;
        case Type::FP64 :
        case Type::I64  :
        case Type::UI64 : this->insert(block.ptr, 8, out);
                          break;

        case Type::Bool : val_bool = *block.ptr ? 0x01 : 0x00;
                          this->insert(&val_bool, 1, out);
                          break;

        case Type::Null : break;

        default         : assert(false && "Invalid type.");
                          break;
    }
}

void PBson::insert_string(MemBlock& block, Out& out)
{
    uint32_t len = block.size + 1;
    this->insert((const uint8_t*)&len, 4, out);

    this->insert(block.ptr, block.size, out);

    uint8_t string_terminator = '\0';
    this->insert(&string_terminator, 1, out);
}

void PBson::insert_binary(MemBlock& block, Out& out)
{
    uint32_t size = block.size;
    this->insert((const uint8_t*)&size, 4, out);

    this->insert(&Bson_Sub_binary, 1, out);
    this->insert(block.ptr, size, out);
}

/* Parse in ***************************************************************/

Parser::SourceSeg PBson::parse_in(In& source)
{
    if(this->type_stack.size() < 1) {
        /* skip document size */
        source.move(4, error);
        Type type = Type::Object;
        this->type_stack.push(type);

        return SourceSeg(type);
    }

    auto bson_prefix = source.cast_move<uint8_t>(error);

    if(bson_prefix == Bson_Scope_End) {
        if(this->type_stack.size() < 1) {
            error();
        }

        this->type_stack.pop();

        return SourceSeg(Type::Scope_End);
    }

    auto name = source.read_block_until(error, '\0');

    if(this->type_stack.top() == Type::Array) {
        /* discard Bson array index */
        name.size = 0;
    }

    if(name.size > 0 && source.is_streaming()) {
        if(this->buffer.size() < name.size) {
            this->buffer.resize(name.size);
        }
        memcpy(&buffer[0], name.ptr, name.size);
        name.ptr = &buffer[0];
    }

    /* skip string terminator */
    source.move(1, error);

    /* new scope */
    if(bson_prefix == Bson_Scope_Container || bson_prefix == Bson_Scope_Object) {
        Type type = bson_prefix == Bson_Scope_Container ? Type::Array : Type::Object;
        this->type_stack.push(type);
        /* skip document size */
        source.move(4, error);

        return SourceSeg(Value(type), name);
    }

    auto value = is_string_prefix(bson_prefix) ? read_string(source)
                    : bson_prefix == Bson_Binary ? read_binary(source)
                    : read_value(source, bson_prefix);

    return SourceSeg(value, name, source.is_streaming());
}

namespace {

    Value read_binary(In& source)
    {
        auto size = source.cast_move<uint32_t>(error);
        /* skip bson buffer subtype */
        source.move(1, error);
        auto block = source.read_block(size, error);

        return Value(block, Type::Binary);
    }

    Value read_string(In& source)
    {
        auto size = source.cast_move<uint32_t>(error);
        /* strings are always null-terminated */
        if(size < 1) {
            error();
        }
        auto block = source.read_block(size, error);
        /* trim away string terminator */
        block.size -= 1;

        return Value(block, Encoding::UTF8);
    }

    Value read_value(In& source, uint8_t bson_prefix)
    {
        if(bson_prefix > sizeof(bson_to_srl)) {
            throw Exception("Unable to parse BSON source. Unknown BSON prefix.");
        }

        auto type = bson_to_srl[bson_prefix];
        auto size = TpTools::get_size(type);
        auto block = source.read_block(size, error);

        return Value(block, type);
    }
}
