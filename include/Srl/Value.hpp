#ifndef SRL_VALUE_HPP
#define SRL_VALUE_HPP

#include "Value.h"
#include "Resolve.h"
#include "Storage.h"
#include "String.h"
#include "TpTools.hpp"

namespace Srl {

    inline Value Value::From_Type(const Lib::MemBlock&block, Type type)
    {
        assert(TpTools::is_literal(type));

        Value val(block, type);
        val.block.move_to_local();

        return val;
    }

    template<class T> T Value::unwrap()
    {
        T r;
        this->paste(r);
        return std::move(r);
    }

    template<class T> void Value::paste(T& o)
    {
        static_assert( TpTools::is_literal(Lib::Switch<T>::type)
                       || Lib::Switch<T>::type == Type::String
                       || Lib::Switch<T>::type == Type::Binary,

            "Srl error. Cannot unwrap type."
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
