#ifndef SRL_BLOCKS_H
#define SRL_BLOCKS_H

#include "Common.h"
#include "Type.h"
#include "Enums.h"

namespace Srl {

    namespace Lib {

        struct MemBlock {

            MemBlock() : ptr(nullptr), size(0) { }
            MemBlock(const uint8_t* ptr_, size_t size_) : ptr(ptr_), size(size_) { }

            const uint8_t* ptr;
            size_t         size;
        };

        constexpr size_t max_packed_block_size() { return sizeof(size_t); }

        struct PackedBlock {

            PackedBlock(const Lib::MemBlock& block, Type type_, Encoding encoding_)
                : extern_data(block.ptr), size(block.size), type(type_), encoding(encoding_) { }

            union {
                uint8_t        local_data[max_packed_block_size()];
                const uint8_t* extern_data;
            };

            uint32_t size;
            Type     type;
            Encoding encoding;
            bool     stored_local = false;


            const uint8_t* data() const
            {
                return this->stored_local ? this->local_data : extern_data;
            }

            void move_to_local()
            {
                assert(this->size <= max_packed_block_size() && !this->stored_local);

                auto* tmp = this->extern_data;
                for(auto i = 0U; i < this->size; i++) {
                    this->local_data[i] = tmp[i];
                }
                this->stored_local = true;
            }
        };
    }
}

#endif
