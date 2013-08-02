#ifndef SRL_STRING_HPP
#define SRL_STRING_HPP

#include "Tools.h"
#include "String.h"

namespace Srl {

    template<class TChar>
    std::basic_string<TChar> String::unwrap(bool throw_error) const
    {
        return this->block.size < 1 ? std::basic_string<TChar>()
            : Tools::conv_charset<TChar>(*this, throw_error);
    }

    inline bool String::operator== (const String& a) const
    {
        const auto cmp = [this](const uint8_t* mem, size_t sz) {
            return this->block.size == sz && memcmp(this->block.data(), mem, sz) == 0;
        };

        if(a.block.encoding == this->block.encoding) {
            return cmp(a.block.data(), a.block.size);

        } else {
            auto tmp = Tools::conv_charset(this->block.encoding, a, false);
            return cmp(tmp.data(), tmp.size());
        }
    }
}

#endif
