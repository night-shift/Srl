#ifndef SRL_VALUE_H
#define SRL_VALUE_H

#include "Enums.h"
#include "Type.h"
#include "Blocks.h"

namespace Srl {

    class Value {

    public:
        Value(Type type_)
            : val_type(type_), str_encoding(Encoding::Unknown), data_block({ nullptr, 0 }) { }

        Value(const Lib::MemBlock& data_, Type type_)
            : val_type(type_), str_encoding(Encoding::Unknown), data_block(data_) { }

        Value(const Lib::MemBlock& data_, Encoding encoding_)
            : val_type(Type::String), str_encoding(encoding_), data_block(data_) { }

        Value(const Lib::MemBlock& data_, Type type_, Encoding encoding_)
            : val_type(type_), str_encoding(encoding_), data_block(data_) { }

        template<typename T> T    unwrap();
        template<typename T> void paste(T& o);

        inline Type           type()     const;
        inline Encoding       encoding() const;
        inline size_t         size()     const;
        inline const uint8_t* data()     const;

    private:
        Type           val_type;
        Encoding       str_encoding;
        Lib::MemBlock  data_block;

    };


}

#endif
