#ifndef SRL_STRING_HPP
#define SRL_STRING_HPP

#include "Tools.h"
#include "String.h"
#include "Storage.h"

namespace Srl {

    template<class TChar>
    std::basic_string<TChar> String::unwrap(bool throw_error) const
    {
        return this->block.size < 1 ? std::basic_string<TChar>()
            : Tools::convert_charset<TChar>(*this, throw_error);
    }

    inline const uint8_t* String::data() const
    {
        return this->block.data();
    }

    inline Encoding String::encoding() const
    {
        return this->block.encoding;
    }

    inline size_t String::size() const
    {
        return this->block.size;
    }

    inline bool String::operator== (const String& a) const
    {
        const auto cmp = [this](const uint8_t* mem, size_t sz) {
            return this->block.size == sz && memcmp(this->block.data(), mem, sz) == 0;
        };

        if(a.block.encoding == this->block.encoding) {
            return cmp(a.block.data(), a.block.size);

        } else {
            auto tmp = Tools::convert_charset(this->block.encoding, a, false);
            return cmp(&tmp[0], tmp.size());
        }
    }
}

#endif
