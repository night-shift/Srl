#ifndef SRL_VALUE_HPP
#define SRL_VALUE_HPP

#include "Value.h"
#include "Resolve.h"
#include "Storage.h"

namespace Srl {

    template<typename T> T Value::unwrap()
    {
        T r;
        this->paste(r);
        return std::move(r);
    }

    template<typename T> void Value::paste(T& o)
    {
        static_assert( TpTools::is_literal(Lib::Switch<T>::type)
                       || Lib::Switch<T>::type == Type::String
                       || Lib::Switch<T>::type == Type::Binary,

            "Srl Error. Cannot unwrap type."
        );

        assert(this->data_block.size < 1 || this->data_block.ptr != nullptr);

        Lib::Switch<T>::Paste(o, *this);
    }

    inline const uint8_t* Value::data() const
    {
        return this->data_block.ptr;
    }

    inline size_t Value::size() const
    {
        return this->data_block.size;
    }

    inline Type Value::type() const
    {
        return this->val_type;
    }

    inline Encoding Value::encoding() const
    {
        return this->str_encoding;
    }
}

#endif
