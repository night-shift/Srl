#ifndef SRL_ENUMS_H
#define SRL_ENUMS_H

#include <cstdint>

namespace Srl {

    enum Mode {
        Insert,
        Paste
    };

    enum class Format : uint8_t {
        Binary,
        Text
    };

    enum class Encoding : uint8_t {
        Unknown,
        UTF8,
        UTF16,
        UTF32
    };
}

#endif
