#ifndef SRL_AUX_H
#define SRL_AUX_H

#include "Blocks.h"
#include "Heap.h"
#include "Common.h"

#include <type_traits>

namespace Srl { namespace Lib { namespace Aux {

    #define srl_unlikely(expr) __builtin_expect((expr), 0)
    #define srl_likely(expr)   __builtin_expect((expr), 1)

    #define srl_noinline __attribute__ ((noinline))

    template<class T>
    T read (const uint8_t* address)
    {
        if((size_t)address % sizeof(T) == 0) {
            return *reinterpret_cast<const T*>(address);

        } else {
            T t;
            memcpy(reinterpret_cast<uint8_t*>(&t), address, sizeof(T));
            return t;
        }
    }

    template<class = void>
    constexpr size_t max_len (size_t max) { return max; }

    template<class Sub, class Token, class... Tail>
    constexpr size_t max_len (size_t max = 0)
    {
        return max_len<Tail...>(std::tuple_size<Token>::value > max ? std::tuple_size<Token>::value : max);
    }

    constexpr size_t dec_saturate(size_t n)
    {
        return n > 0 ? n - 1 : 0;
    }

    template<size_t N, class T> bool comp (const uint8_t* a, const T* b)
    {
        return N == 0 || (*a == *b && comp<dec_saturate(N)>(a + 1, b + 1));
    }
    
    template<size_t N> void copy (const uint8_t* src, uint8_t* dst)
    {
        *dst = *src;
        copy<N - 1>(src + 1, dst + 1);
    }

    template<> inline void copy<0> (const uint8_t*, uint8_t*) { }

    inline void copy_to_vec (std::vector<uint8_t>& vec, const uint8_t* src, size_t n)
    {
        if(vec.size() < n) vec.resize(n);
        memcpy(vec.data(), src, n);
    }

    inline void copy_to_vec (std::vector<uint8_t>& vec, const MemBlock& block)
    {
        copy_to_vec(vec, block.ptr, block.size);
    }

    inline MemBlock copy (std::vector<uint8_t>& vec, const MemBlock& block)
    {
        copy_to_vec(vec, block);
        return { vec.data(), block.size };
    }

    inline MemBlock copy (Heap& heap, const MemBlock& block)
    {
        if(block.size < 1) return block;

        auto* mem = heap.get_mem(block.size);
        memcpy(mem, block.ptr, block.size);

        return { mem, block.size };
    }

    template<class T>
    void clear_stack(std::stack<T>& st)
    {
        while(st.size() > 0) st.pop();
    }

} } }

#endif
