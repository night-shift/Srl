#include "Srl/Hash.h"

using namespace Srl;
using namespace Lib;

namespace {

    template<size_t word_length> struct Arch;

    template<> struct Arch<8> {

        static const uint64_t Mul  = 0xC6A4A7935BD1E995;
        static const uint64_t Base = 0xCBF29CE484222325;
        static const uint64_t Shr  = 47;

        static size_t hash_rem (const uint8_t* bytes, size_t nbytes, uint64_t hash)
        {
            switch(nbytes % 8) {
                case 7 : hash ^= (uint64_t)bytes[6] << 48;
                case 6 : hash ^= (uint64_t)bytes[5] << 40;
                case 5 : hash ^= (uint64_t)bytes[4] << 32;
                case 4 : hash ^= (uint64_t)bytes[3] << 24;
                case 3 : hash ^= (uint64_t)bytes[2] << 16;
                case 2 : hash ^= (uint64_t)bytes[1] << 8;
                case 1 : hash ^= (uint64_t)bytes[0];
                            hash *= Mul;
            };

            hash ^= hash >> Shr;
            hash *= Mul;
            hash ^= hash >> Shr;

            return hash;
        }
    };

    template<> struct Arch<4> {
        static const uint32_t Mul  = 0x5BD1E995;
        static const uint32_t Base = 0x811C9DC5;
        static const uint32_t Shr  = 24;

        static size_t hash_rem (const uint8_t* bytes, size_t nbytes, uint32_t hash)
        {
            switch(nbytes % 4) {
                case 3 : hash ^= (uint32_t)bytes[2] << 16;
                case 2 : hash ^= (uint32_t)bytes[1] << 8;
                case 1 : hash ^= (uint32_t)bytes[0];
                        hash *= Mul;
            };

            hash ^= hash >> 13;
            hash *= Mul;
            hash ^= hash >> 15;

            return hash;
        }
    };

    size_t murmur_hash2(const uint8_t * bytes, size_t nbytes)
    {
        const auto wordlen = sizeof(size_t);

        const auto mul  = Arch<wordlen>::Mul;
        const auto shr  = Arch<wordlen>::Shr;
        const auto base = Arch<wordlen>::Base;

        size_t hash = base ^ (nbytes * mul);

        for(; nbytes >= wordlen; nbytes -= wordlen, bytes += wordlen) {

            auto b = Aux::read<size_t>(bytes);

            b *= mul;
            b ^= b >> shr;
            b *= mul;

            hash ^= b;
            hash *= mul;
        }

        return Arch<wordlen>::hash_rem(bytes, nbytes, hash);
    }
}
size_t Aux::hash_fnc(const uint8_t* bytes, size_t nbytes)
{
    return murmur_hash2(bytes, nbytes);
}
