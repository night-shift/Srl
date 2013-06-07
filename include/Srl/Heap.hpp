#ifndef SRL_HEAP_HPP
#define SRL_HEAP_HPP

#include "Common.h"
#include "Heap.h"

namespace Srl { namespace Lib {

    inline Heap::Segment::~Segment()
    {
        if(this->data != nullptr) {
            delete[] this->data;
            this->data = nullptr;
        }
    }

    inline Heap::Segment& Heap::Segment::operator=(const Segment& s)
    {
        if(this->data != nullptr) {
            delete[] this->data;
        }
        if(s.data != nullptr) {
            this->data = new uint8_t[s.size];
            memcpy(this->data, s.data, s.size);
        }
        this->fill = s.fill;
        this->size = s.size;

        return *this;
    }

    inline Heap::Segment& Heap::Segment::operator=(Segment&& s)
    {
        if(this->data != nullptr) {
            delete[] this->data;
        }
        this->data = s.data;
        this->size = s.size;
        this->fill = s.fill;

        s.data = nullptr;

        return *this;
    }

    template<class T>
    uint8_t* Heap::alloc_type()
    {
        auto* ptr = this->get_mem(sizeof(T) + alignof(T));

        return ptr + alignof(T) - (size_t)ptr % alignof(T);
    }

    inline void Heap::set_seg_fill()
    {
        if(this->crr_seg != nullptr) {
            this->crr_seg->fill = this->cap - this->mem_left;
        }
    }

    inline uint8_t* Heap::get_mem(size_t nbytes)
    {
        if(this->mem_left < nbytes) {
            this->alloc(nbytes);
        }

        this->mem_left -= nbytes;
        this->mem += nbytes;

        return this->mem - nbytes;
    }

    inline MemBlock Heap::copy(const MemBlock& block)
    {
        auto* ptr = this->get_mem(block.size);
        memcpy(ptr, block.ptr, block.size);

        return { ptr, block.size };
    }

    inline void Heap::clear()
    {
        this->segments.clear();
        this->mem_left = 0;
        this->cap = 0;
    }

} }

#endif
