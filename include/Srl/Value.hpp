#ifndef SRL_VALUE_HPP
#define SRL_VALUE_HPP

#include "Value.h"
#include "Resolve.h"
#include "Environment.h"
#include "String.h"
#include "TpTools.h"

namespace Srl {

    template<class T> Value::Value(const T& val)
        : block(sizeof(T), TpTools::SrlType<T>::type, Encoding::Unknown)
    {
        static_assert(Lib::is_numeric<T>::value, "Cannot create Value from non-numeric type.");

        const auto tp = TpTools::SrlType<T>::type;

        if(tp == Type::FP64) {
            if((float)val == val) {
                this->block.fp32 = val;
                this->block.size = 4;
                this->block.type = Type::FP32;

            } else {
                this->block.fp64 = val;
            }

        } else if(tp == Type::FP32) {
            this->block.fp32 = val;

        } else if(TpTools::is_signed(tp)) {
            this->block.i64 = val;

        } else {
            this->block.ui64 = val;
        }

        this->block.stored_local = true;
    }

    template<class T> T Value::unwrap()
    {
        T r;
        this->paste(r);
        return r;
    }

    template<class T> void Value::paste(T& o)
    {
        static_assert( TpTools::is_scalar(Lib::Switch<T>::type)
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
        return this->name_ptr
            ? *this->name_ptr
            : Lib::Environment::EmptyString;
    }

    inline const Lib::PackedBlock& Value::pblock() const
    {
        return this->block;
    }
}

#endif
