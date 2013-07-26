#include "Srl/Srl.h"

#include <limits>
#include <math.h>

using namespace std;
using namespace Srl;
using namespace Lib;
using namespace Tools;

namespace {

    const char str_true[]  = "true";
    const char str_false[] = "false";
    const char str_null[]  = "null";

    tuple<double, bool> string_to_double(const uint8_t* str, size_t str_len)
    {
        const size_t buffer_size = 64;
        char buffer[buffer_size];

        double val;

        const auto apply = [&val, str, str_len](char* ptr_mem) -> bool {
            char* end = nullptr;
            memcpy(ptr_mem, str, str_len);
            ptr_mem[str_len] = '\0';
            val = strtod(ptr_mem, &end);

            return end == ptr_mem + str_len;
        };

        if(str_len + 1 < buffer_size) {
            return make_tuple(val, apply(buffer));

        } else {
            vector<char> vec_buffer(str_len + 1);
            return make_tuple(val, apply(vec_buffer.data()));
        }
    }

    tuple<uint64_t, bool, bool> string_to_int(const uint8_t* str, size_t str_len)
    {
        size_t   idx = 0;
        uint64_t val = 0;
        bool is_signed = false;

        if(str_len > 0 && *str == '-') {
            if(str_len < 2) {
                /* invalid string */
                return make_tuple(val, false, false);
            }
            is_signed = true;
            idx = 1;
            str++;

        }

        uint64_t tmp = 0;
        for(char c = *str; idx < str_len;
            c = *++str, idx++) {

            if(c >= '0' && c <= '9') {
                tmp = val * 10 + c - '0';
                if(tmp < val) {
                    /* overflow */
                    break;
                }
                val = tmp;
            } else {
                break;
            }
        }

        bool success = idx >= str_len && (!is_signed || val <= numeric_limits<int64_t>::max());

        return make_tuple(val, is_signed, success);
    }

    template<typename TCharAr>
    bool compare_strings(const uint8_t* str, size_t str_len, const TCharAr& to_compare, size_t offset = 0)
    {
        assert(str_len >= offset);

        return str_len == sizeof(TCharAr) - 1 &&
            memcmp(to_compare + offset, str + offset, str_len - offset) == 0;
    }

    pair<bool, Value> string_to_type_converted(const uint8_t* str, size_t str_len, Type hint)
    {
        Tools::trim_space(str, str_len);

        if(str_len < 1) {
            return { false, Value(Type::Null) };
        }

        if(str_len >= 4 && str[0] >= 'f') {

            if((str[0] == 't' || str[0] == 'T') && compare_strings(str, str_len, str_true, 1)) {
                return { true, true };
            }

            if((str[0] == 'f' || str[0] == 'F') && compare_strings(str, str_len, str_false, 1)) {
                return { true, false };
            }

            if((str[0] == 'n' || str[0] == 'N') && compare_strings(str, str_len, str_null, 1)) {
                return { true, Value(Type::Null) };
            }
        }

        bool success = false;

        if(!TpTools::is_fp(hint)) {
            bool is_signed;
            uint64_t i;

            tie(i, is_signed, success) = string_to_int(str, str_len);

            if(success) {
                return is_signed ? make_pair(true, Value(-(int64_t)i)) : make_pair(true, Value(i));
            }
        }

        double fp;
        tie(fp, success) = string_to_double(str, str_len);

        return success ? make_pair(true, Value(fp)) : make_pair(false, Value(Type::Null));
    }

    void copy_to_vector(vector<uint8_t>& vec, const uint8_t* data, size_t size)
    {
        if(vec.size() < size) {
            vec.resize(size);
        }
        memcpy(vec.data(), data, size);
    }

    template<Type TP>
    typename enable_if<TpTools::is_num(TP) && !TpTools::is_fp(TP), size_t>::type
    conv_type(const Value& value, vector<uint8_t>& out)
    {
        typedef typename TpTools::Real<TP>::type T;
        const auto buffer_size = size_t(numeric_limits<T>::digits10) + 1 + TpTools::is_signed(TP);

        uint8_t buffer[buffer_size];

        bool negative = TpTools::is_signed(TP) && value.pblock().i64 < 0;

        uint64_t val = negative ? -value.pblock().i64 : value.pblock().ui64;

        if(val == 0) {
            if(out.size() < 1) out.resize(1);
            out[0] = '0';
            return 1;
        }

        size_t str_len = 0;
        auto* ptr = buffer + buffer_size - 1;

        for(; val > 0U;
            val /= 10U, ptr--, str_len++) {

            *ptr = '0' + val % 10;
        }

        if(negative) {
            *(ptr) = '-';
            str_len++;
        } else {
            ptr++;
        }

        copy_to_vector(out, ptr, str_len);

        return str_len;
    }

    template<Type type> typename enable_if<TpTools::is_fp(type), size_t>::type
    conv_type(const Value& value, vector<uint8_t>& out)
    {
        double val = type == Type::FP64 ? value.pblock().fp64 : value.pblock().fp32;

        if(floor(val) == val && val <= numeric_limits<int64_t>::max() && val >= numeric_limits<int64_t>::min()) {

            return conv_type<Type::I64>((int64_t)val, out);
        }

        int sz_buf = out.size();
        int sz = 64;

        do {
            if(sz_buf < sz + 1) /* string terminator */ {
                sz_buf = sz + 1;
                out.resize(sz_buf);
            }

            sz = snprintf((char*)out.data(), sz_buf, "%f", val);

            if(sz < 0) {
                throw Exception("Error occurred on converting floating point value to string.");
            }

            if(sz > sz_buf)
                printf("%i ", sz);

        } while(sz > sz_buf);

        while(sz > 1 && *(out.data() + sz - 1) == '0') {
            sz--;
        }

        return sz;
    }

    template<Type type> typename enable_if<type == Type::Bool, size_t>::type
    conv_type(const Value& value, vector<uint8_t>& out)
    {
        const char* str = value.pblock().ui64 ? str_true : str_false;
        size_t size = (value.pblock().ui64 ? sizeof(str_true) : sizeof(str_false)) - 1;

        copy_to_vector(out, (const uint8_t*)str, size);

        return size;
    }

    template<Type type>
    typename enable_if<type == Type::Null, size_t>::type
    conv_type(const Value&, vector<uint8_t>& out)
    {
        copy_to_vector(out, (const uint8_t*)str_null, sizeof(str_null) - 1);
        return sizeof(str_null) - 1;
    }
}

void Tools::trim_space(Lib::MemBlock& block)
{
    return trim_space(block.ptr, block.size);
}

void Tools::trim_space(const uint8_t*& str, size_t& str_len)
{
    if(str == nullptr) {
        return;
    }

    while(str_len > 0 && is_space(*str)) {
        str++;
        str_len--;
    }

    while(str_len > 0 && is_space(*(str + str_len - 1))) {
        str_len--;
    }
}

pair<bool, Value> Tools::string_to_type(const String& string_wrap, Type hint)
{
    if(string_wrap.encoding() == Encoding::UTF8) {
        return string_to_type_converted(string_wrap.data(), string_wrap.size(), hint);

    } else {
        auto vec = Tools::convert_charset(Encoding::UTF8, string_wrap, false);
        if(vec.size() < 1) {
            return make_pair(false, Value(Type::Null));
        }

        return string_to_type_converted(vec.data(), vec.size(), hint);
    }
}

#define SRL_TYPE_TO_STR(id, idx, real, size) \
    case Type::id : return conv_type<Type::id>(value, out);

size_t Tools::type_to_string(const Value& value, vector<uint8_t>& out)
{
    switch(value.type()) {
        /* defined in Type.h */
        SRL_TYPES_SCALAR(SRL_TYPE_TO_STR)

        default : return 0;
    }
}

#undef SRL_TYPE_TO_STR

string Tools::type_to_string(const Value& value)
{
    vector<uint8_t> buffer;
    auto size = Tools::type_to_string(value, buffer);

    return size > 0 ? string((const char*)buffer.data(), size) : string();
}

vector<uint8_t> Tools::convert_charset(Encoding target_encoding, const String& string_wrap, bool throw_error)
{
    vector<uint8_t> buffer;
    auto converted_total = convert_charset(target_encoding, string_wrap, buffer, throw_error);
    buffer.resize(converted_total);

    return move(buffer);
}

/* At time of writing standard code conversion facets aren't implemented in libstdc++, so:
 * TODO convert with codecvt */
#include <iconv.h>
namespace {

    const char* get_charset_string(Encoding encoding)
    {
        static const char str_utf8[] = "UTF-8", str_utf16[] = "UTF-16LE", str_utf32[] = "UTF-32LE";

        const char* rslt = nullptr;

        switch(encoding) {
            case Encoding::UTF8    : rslt = str_utf8;  break;
            case Encoding::UTF16   : rslt = str_utf16; break;
            case Encoding::UTF32   : rslt = str_utf32; break;
            case Encoding::Unknown : rslt = nullptr;   break;
        }

        return rslt;
    }
}

size_t Tools::convert_charset(Encoding target_encoding, const String& str_wrap,
                              vector<uint8_t>& buffer,  bool throw_error, size_t buffer_index)
{
    if(str_wrap.size() < 1) {
        return 0;
    }

    auto src_encoding = str_wrap.encoding();

    auto* cs_from = get_charset_string(src_encoding);
    auto* cs_to   = get_charset_string(target_encoding);

    if(cs_from == cs_to) {
        auto seg = str_wrap.size() + buffer_index;
        if(buffer.size() < seg) {
            buffer.resize(seg);
        }
        memcpy(&buffer[buffer_index], str_wrap.data(), str_wrap.size());

        return str_wrap.size();
    }

    if(cs_from == nullptr || cs_to == nullptr) {
        if(throw_error) {
            throw Exception("Unable to convert charset. Unknown encoding.");
        }
        return 0;
    }

    iconv_t desc = iconv_open(cs_to, cs_from);

    if(desc == (iconv_t)(-1)) {
        if(throw_error) {
            throw Exception("Unable to convert charset. Failed to open iconv descriptor.");
        }
        return 0;
    }

    size_t left_in = str_wrap.size();
    size_t reserve = left_in * (
        target_encoding < src_encoding ? 1 : src_encoding == Encoding::UTF16 ? 2 : 4
    );

    if(buffer_index + reserve > buffer.size()) {
        buffer.resize(buffer_index + reserve);
    }
    size_t left_out = buffer.size() - buffer_index;

    auto* src = (char*)str_wrap.data();
    size_t converted = 0;

    while(true) {
        auto* dst = (char*)&buffer[buffer_index + converted];
        converted = iconv(desc, &src, &left_in, &dst, &left_out);

        if(converted == 0) {
            /* all characters converted */
            break;
        }

        if(errno == EINVAL) {
            /* incomplete byte-sequence encountered, not exception-worthy */
            break;
        }

        if(errno == EILSEQ) {
            iconv_close(desc);
            if(throw_error) {
                throw Exception("Unable to convert charset. Invalid byte sequence.");
            }
            return 0;
        }

        if (errno == E2BIG) {
            /* output buffer not big enough */
            auto current = buffer.size();
            auto new_size = buffer.size() + left_in * 4;
            buffer.resize(new_size);
            converted = current - left_out;
            left_out = new_size - converted;
        }
    }

    iconv_close(desc);

    return buffer.size() - buffer_index - left_out;
}
