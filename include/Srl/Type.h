#ifndef SRL_TYPE_H
#define SRL_TYPE_H

#include "Common.h"

namespace Srl {

    static_assert(sizeof(float) == 4 && sizeof(double) == 8, "Srl error. Floating point format not supported.");

    #define SRL_TYPE_ENUM_ENTRY(name, value, type, size) name = value,
    /* enum-name, enum-value, associated-type, size of associated-type */
    #define SRL_TYPES_SCALAR(FNC) \
        FNC (Bool, 0,  bool,      1) \
        FNC (I8,   1,  int8_t,    1) \
        FNC (UI8,  2,  uint8_t,   1) \
        FNC (I16,  3,  int16_t,   2) \
        FNC (UI16, 4,  uint16_t,  2) \
        FNC (I32,  5,  int32_t,   4) \
        FNC (UI32, 6,  uint32_t,  4) \
        FNC (I64,  7,  int64_t,   8) \
        FNC (UI64, 8,  uint64_t,  8) \
        FNC (FP32, 9,  float,     4) \
        FNC (FP64, 10, double,    8) \
        FNC (Null, 11, std::nullptr_t, 0)

    #define SRL_TYPES_NON_SCALAR(FNC) \
        FNC (String,     12, 0, 0) \
        FNC (Binary,     13, 0, 0) \
        FNC (Object,     14, 0, 0) \
        FNC (Array,      15, 0, 0) \
        FNC (Scope_End,  16, 0, 0)

    enum class Type : uint8_t {
        SRL_TYPES_SCALAR(SRL_TYPE_ENUM_ENTRY)
        SRL_TYPES_NON_SCALAR(SRL_TYPE_ENUM_ENTRY)
    };

    #undef SRL_TYPE_ENUM_ENTRY
} 

#endif
