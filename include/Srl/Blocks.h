#ifndef SRL_BLOCKS_H
#define SRL_BLOCKS_H

#include "Common.h"

namespace Srl {

    namespace Lib {

        struct MemBlock {

            MemBlock() : ptr(nullptr), size(0) { }
            MemBlock(const uint8_t* ptr_, size_t size_) : ptr(ptr_), size(size_) { }

            const uint8_t* ptr;
            size_t         size;
        };
    }
}

#endif
