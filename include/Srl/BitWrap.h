#ifndef SRL_BIT_WRAP_H
#define SRL_BIT_WRAP_H

#include "Blocks.h"
#include "Common.h"

#include <functional>

namespace Srl {

    /* usage:
         * On Insert: Node stores memory pointed by data of size 'size'.
         *
         * On Paste: alloc function will be called with the restored size of the memory block as argument.
         * Function expects as result a pointer to memory where it can copy the restored data block to.
         */

    struct BitWrap {

        BitWrap(const uint8_t* ptr, size_t size, const std::function<uint8_t*(size_t)>& alloc_)
            : data(ptr, size), alloc(alloc_) { }

        BitWrap(const uint8_t* ptr, size_t size)
            : data(ptr, size) { }

        BitWrap(const std::function<uint8_t*(size_t)>& alloc_)
            : alloc(alloc_) { }

        const Lib::MemBlock data;
        const std::function<uint8_t*(size_t)> alloc;
    };

    template<class T> struct VecWrap {

        VecWrap(std::vector<T>& vec_)
            : vec(&vec_), bitwrap(
                (uint8_t*)vec->data(), vec->size() * sizeof(T),
                [this](size_t sz)
                {
                    vec->resize(sz / sizeof(T));
                    return (uint8_t*)vec->data(); 
                }) 
        {  }

        std::vector<T>* vec;
        BitWrap         bitwrap;
    };

}

#endif
