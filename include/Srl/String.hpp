#ifndef SRL_STRING_HPP
#define SRL_STRING_HPP

#include "Tools.h"
#include "String.h"
#include "Storage.h"

namespace Srl {

    template<class TChar>
    std::basic_string<TChar> String::unwrap(bool throw_error) const
    {
        return this->data_block.size < 1 ? std::basic_string<TChar>()
            : Tools::convert_charset<TChar>(*this, throw_error);
    }

    inline const uint8_t* String::data() const
    {
        return this->data_block.ptr;
    }

    inline Encoding String::encoding() const
    {
        return this->enc;
    }

    inline size_t String::size() const
    {
        return this->data_block.size;
    }
}

#endif
