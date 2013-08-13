#ifndef SRL_STRING_H
#define SRL_STRING_H

#include "Resolve.h"
#include "Blocks.h"
#include "Enums.h"
#include "Value.h"
#include "Type.h"

namespace Srl {

    namespace Lib { struct Environment; }

    class String {

        friend struct Lib::Environment;

    public:
        template<class TString>
        String(const TString& str) : String(Lib::wrap_string(str)) { }

        String() : block(Lib::MemBlock(), Type::String,Encoding::UTF8) { }

        String(const std::pair<Encoding, Lib::MemBlock>& pair)
            :  block(pair.second, Type::String, pair.first) { }

        String(Lib::MemBlock data_, Encoding encoding_ = Encoding::UTF8)
            : block(data_, Type::String, encoding_ ) { }

        String(const uint8_t* ptr, size_t size_, Encoding encoding_)
            : block({ ptr, size_ }, Type::String, encoding_) { }

        template<class TChar = char>
        std::basic_string<TChar> unwrap(bool throw_error = true) const;

        inline bool operator== (const String& a) const;

        inline const uint8_t* data()     const { return this->block.data();   }
        inline size_t         size()     const { return this->block.size;     }
        inline Encoding       encoding() const { return this->block.encoding; }

    private:
        Lib::PackedBlock block;
    };
}

#endif
