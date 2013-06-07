#ifndef SRL_STRING_H
#define SRL_STRING_H

#include "Resolve.h"
#include "Blocks.h"
#include "Enums.h"
#include "Value.h"

namespace Srl {

    class String {

    public:
        template<class TString>
        String(const TString& str) : String(Lib::wrap_string(str)) { }

        String() : enc(Encoding::UTF8) { }

        String(const std::pair<Encoding, Lib::MemBlock>& pair)
            :  data_block(pair.second), enc(pair.first) { }

        String(Lib::MemBlock data_, Encoding encoding_)
            : data_block(data_), enc(encoding_) { }

        String(const uint8_t* ptr, size_t size_, Encoding encoding_)
            : data_block(ptr, size_), enc(encoding_) { }

        template<class TChar>
        std::basic_string<TChar> unwrap(bool throw_error = true) const;

        inline const uint8_t* data()     const;
        inline size_t         size()     const;
        inline Encoding       encoding() const;

    private:
        Lib::MemBlock   data_block;
        Encoding        enc;
    };
}

#endif
