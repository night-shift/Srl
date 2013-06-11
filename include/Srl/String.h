#ifndef SRL_STRING_H
#define SRL_STRING_H

#include "Resolve.h"
#include "Blocks.h"
#include "Enums.h"
#include "Value.h"
#include "Type.h"

namespace Srl {

    namespace Lib { class Storage; }

    class String {

        friend class Lib::Storage;

    public:
        template<class TString>
        String(const TString& str) : String(Lib::wrap_string(str)) { }

        String() : block(Lib::MemBlock(), Type::String,Encoding::UTF8) { }

        String(const std::pair<Encoding, Lib::MemBlock>& pair)
            :  block(pair.second, Type::String, pair.first) { }

        String(Lib::MemBlock data_, Encoding encoding_)
            : block(data_, Type::String, encoding_ ) { }

        String(const uint8_t* ptr, size_t size_, Encoding encoding_)
            : block({ ptr, size_ }, Type::String, encoding_) { }

        template<class TChar>
        std::basic_string<TChar> unwrap(bool throw_error = true) const;

        inline bool operator== (const String& a) const;

        inline const uint8_t* data()     const;
        inline size_t         size()     const;
        inline Encoding       encoding() const;

    private:
        Lib::PackedBlock block;
    };
}

#endif
