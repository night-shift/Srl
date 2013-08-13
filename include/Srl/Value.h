#ifndef SRL_VALUE_H
#define SRL_VALUE_H

#include "Enums.h"
#include "Type.h"
#include "Blocks.h"

namespace Srl {

    class String;
    namespace Lib { class Environment; }

    class Value {

        friend class Lib::Environment;

    public:
        template<class T> Value (const T& val);

        Value(Type type_ = Type::Null)
            : block({ nullptr, 0 }, type_, Encoding::Unknown) { }

        Value(const Lib::MemBlock& data_, Type type_)
            : block(data_, type_, Encoding::Unknown) { }

        Value(const Lib::MemBlock& data_, Encoding encoding_)
            : block(data_, Type::String, encoding_) { }

        Value(const Lib::MemBlock& data_, Type type_, Encoding encoding_)
            : block(data_, type_, encoding_) { }

        template<class T> T    unwrap();
        template<class T> void paste(T& o);

        inline Type                    type()     const;
        inline Encoding                encoding() const;
        inline size_t                  size()     const;
        inline const uint8_t*          data()     const;
        inline const String&           name()     const;
        inline const Lib::PackedBlock& pblock()   const;

    private:
        Value(const Lib::PackedBlock& data_) : block(data_) { }

        Lib::PackedBlock block;
        const String*    name_ptr;
    };
}

#endif
