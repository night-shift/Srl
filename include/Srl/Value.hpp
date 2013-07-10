#ifndef SRL_VALUE_HPP
#define SRL_VALUE_HPP

#include "Value.h"
#include "Resolve.h"
#include "Storage.h"
#include "String.h"

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

        assert(this->block.size < 1 || this->block.data() != nullptr);

        Lib::Switch<T>::Paste(o, *this);
    }

    inline const uint8_t* Value::data() const
    {
        return this->block.data();
    }

    inline size_t Value::size() const
    {
        return this->block.size;
    }

    inline Type Value::type() const
    {
        return this->block.type;
    }

    inline Encoding Value::encoding() const
    {
        return this->block.encoding;
    }

    inline const String& Value::name() const
    {
        return *this->name_ptr;
    }
}

#endif
