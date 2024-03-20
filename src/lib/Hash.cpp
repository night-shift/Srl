#include "Srl/Hash.h"

using namespace Srl;
using namespace Lib;

namespace {

    const uint64_t fnv1a_prime = 0x100000001B3;
    const uint64_t fnv1a_base  = 0xCBF29CE484222325;

    template<size_t N> void fnv1a_unfold(const uint8_t* bytes, uint64_t& hash)
    {
        hash = (hash ^ *bytes) * fnv1a_prime;
        fnv1a_unfold<N - 1>(bytes + 1, hash);
    }

    template<> inline void fnv1a_unfold<0>(const uint8_t*, uint64_t&) { }

    uint64_t fnv1a(const uint8_t* bytes, size_t nbytes)
    {
        auto hash = fnv1a_base;

        for(auto unroll = nbytes / 8; unroll > 0;
            unroll--, bytes += 8) {

            fnv1a_unfold<8>(bytes, hash);
        }

        auto rem = nbytes % 8;

        switch(rem) {
            case 0 : break;
            case 1 : fnv1a_unfold<1>(bytes, hash); break;
            case 2 : fnv1a_unfold<2>(bytes, hash); break;
            case 3 : fnv1a_unfold<3>(bytes, hash); break;
            case 4 : fnv1a_unfold<4>(bytes, hash); break;
            case 5 : fnv1a_unfold<5>(bytes, hash); break;
            case 6 : fnv1a_unfold<6>(bytes, hash); break;
            case 7 : fnv1a_unfold<7>(bytes, hash); break;
        }

        return hash;
    }
}

uint64_t Aux::hash_fnc(const uint8_t* bytes, size_t nbytes)
{
    return fnv1a(bytes, nbytes);
}
