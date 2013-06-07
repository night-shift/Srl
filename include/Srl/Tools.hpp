#ifndef SRL_TOOLS_HPP
#define SRL_TOOLS_HPP

#include "Tools.h"
#include "Resolve.h"
#include "String.h"

namespace Srl { namespace Tools {

    template<class TChar>
    std::basic_string<TChar> convert_charset(const String& string_wrap, bool throw_error)
    {
        static_assert(Lib::is_character<TChar>::value, "Srl Error. Not a character-type.");

        auto target_encoding = Lib::get_encoding<TChar>();

        if(string_wrap.encoding() == target_encoding) {
            return std::basic_string<TChar>(
                reinterpret_cast<const TChar*>(string_wrap.data()),
                string_wrap.size() / sizeof(TChar)
            );
        }

        std::vector<uint8_t> converted = convert_charset(target_encoding, string_wrap, throw_error);
        auto n_chars = converted.size() / sizeof(TChar);

        return n_chars > 0
            ? std::basic_string<TChar>(reinterpret_cast<const TChar*>(&converted[0]), n_chars)
            : std::basic_string<TChar>();
    }

    constexpr bool is_space(uint8_t token)
    {
        return token == ' ' || (token >= 9 && token <= 13);
    }
} }

#endif
