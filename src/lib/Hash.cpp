#include "Srl/Hash.h"

using namespace Srl;
using namespace Lib;

namespace {

    uint64_t murmur_hash2(const uint8_t * bytes, size_t nbytes)
    {
        const auto wordlen = sizeof(size_t);

        const uint64_t mul  = 0xC6A4A7935BD1E995;
        const uint64_t base = 0xCBF29CE484222325;
        const uint64_t shr  = 47;

        size_t hash = base ^ (nbytes * mul);

        for(; nbytes >= wordlen; nbytes -= wordlen, bytes += wordlen) {

            auto b = Aux::read<size_t>(bytes);

            b *= mul;
            b ^= b >> shr;
            b *= mul;

            hash ^= b;
            hash *= mul;
        }

        switch(nbytes % 8) {
            case 7 : hash ^= (uint64_t)bytes[6] << 48;
            case 6 : hash ^= (uint64_t)bytes[5] << 40;
            case 5 : hash ^= (uint64_t)bytes[4] << 32;
            case 4 : hash ^= (uint64_t)bytes[3] << 24;
            case 3 : hash ^= (uint64_t)bytes[2] << 16;
            case 2 : hash ^= (uint64_t)bytes[1] << 8;
            case 1 : hash ^= (uint64_t)bytes[0];
                     hash *= mul;
        };

        hash ^= hash >> shr;
        hash *= mul;
        hash ^= hash >> shr;

        return hash;
    }
}

uint64_t Aux::hash_fnc(const uint8_t* bytes, size_t nbytes)
{
    return murmur_hash2(bytes, nbytes);
}
