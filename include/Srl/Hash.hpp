#ifndef SRL_HASH_HPP
#define SRL_HASH_HPP

#include "Hash.h"

namespace Srl { namespace Tools {

    namespace Aux {

        template<size_t N> void fnv1a_unfold(const uint8_t* bytes, size_t& hash)
        {
            hash = (hash ^ *bytes) * HashParamsFnv1<sizeof(size_t)>::Prime;
            fnv1a_unfold<N - 1>(bytes + 1, hash);
        }

        template<> inline void fnv1a_unfold<0>(const uint8_t*, size_t&) { }
    }

    template<size_t NBytes>
    size_t hash_fnv1a(const uint8_t* bytes, size_t hash_base)
    {
        Aux::fnv1a_unfold<NBytes>(bytes, hash_base);
        return hash_base;
    }

    template<size_t N>
    constexpr size_t hash_fnv1a (const char(&str)[N], size_t hash_base)
    {
        return hash_fnv1a<N>(str, hash_base);
    }

    inline size_t hash_fnv1a(const uint8_t* bytes, size_t nbytes, size_t hash_base)
    {

        for(auto unroll = nbytes / 8; unroll > 0;
            unroll--, bytes += 8) {

            Aux::fnv1a_unfold<8>(bytes, hash_base);
        }

        auto rem = nbytes % 8;

        switch(rem) {
            case 0 : break;
            case 1 : Aux::fnv1a_unfold<1>(bytes, hash_base); break;
            case 2 : Aux::fnv1a_unfold<2>(bytes, hash_base); break;
            case 3 : Aux::fnv1a_unfold<3>(bytes, hash_base); break;
            case 4 : Aux::fnv1a_unfold<4>(bytes, hash_base); break;
            case 5 : Aux::fnv1a_unfold<5>(bytes, hash_base); break;
            case 6 : Aux::fnv1a_unfold<6>(bytes, hash_base); break;
            case 7 : Aux::fnv1a_unfold<7>(bytes, hash_base); break;
        }

        return hash_base;
    }

} }

#endif
