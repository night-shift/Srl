#ifndef SRL_TOOLS_H
#define SRL_TOOLS_H

#include "Common.h"
#include "Enums.h"
#include "Type.h"
#include "Blocks.h"
#include "Value.h"

namespace Srl {

    class String;

namespace Tools {

    std::string type_to_string (const Value& value);
    size_t      type_to_string (const Value& value, std::vector<uint8_t>& out);

    std::pair<bool, Value> string_to_type (const String& string_wrap, Type hint = Type::Null);
    std::pair<bool, Value> string_to_type (const uint8_t* str, size_t str_len, Type hint = Type::Null);

    std::vector<uint8_t> bytes_to_hex (const uint8_t* bytes, size_t nbytes);
    std::vector<uint8_t> hex_to_bytes (const uint8_t* str, size_t str_len);
    void                 hex_to_bytes (uint8_t* dest, size_t dest_size, const uint8_t* str, size_t str_len);

    std::vector<uint8_t> bytes_to_base64 (const uint8_t* bytes, size_t nbytes);
    size_t               bytes_to_base64 (const uint8_t* bytes, size_t nbytes, std::vector<uint8_t>& buffer);

    std::vector<uint8_t> base64_to_bytes (const char* str, size_t str_len);
    void                 base64_to_bytes (uint8_t* dest, size_t dest_size, const char* str, size_t str_len);
    size_t               get_base64_decoded_size (const char* str, size_t str_len);

    std::vector<uint8_t> convert_charset (Encoding target_encoding, const String& string_wrap, bool throw_error = true);

    std::vector<uint8_t> convert_charset (Encoding target_encoding, const char* str, size_t str_len, bool throw_error = true);

    size_t               convert_charset (Encoding target_encoding, const String& string_wrap,
                                          std::vector<uint8_t>& buffer, bool throw_error, size_t buffer_index = 0);
    template<class TChar>
    std::basic_string<TChar> convert_charset (const String& string_wrap, bool throw_error);

    void trim_space (const uint8_t*& str, size_t& str_len);
    void trim_space (Lib::MemBlock& block);

    constexpr bool is_space(uint8_t token);
} }

#endif
