#ifndef SRL_HEAP_HPP
#define SRL_HEAP_HPP

#include "Common.h"
#include "Heap.h"

namespace Srl { namespace Lib {

    template<class T>
    Heap<T>::Segment::~Segment()
    {
        if(this->data != nullptr) {
            operator delete(this->data);
            this->data = nullptr;
        }
    }

    template<class T>
    typename Heap<T>::Segment& Heap<T>::Segment::operator=(const Segment& s)
    {
        if(this->data != nullptr) {
            operator delete(this->data);
        }
        if(s.data != nullptr) {
            this->data = static_cast<T*>(operator new(sizeof(T) * s.size));
            memcpy(this->data, s.data, s.size * sizeof(T));
        }
        this->fill = s.fill;
        this->size = s.size;

        return *this;
    }

    template<class T>
    typename Heap<T>::Segment& Heap<T>::Segment::operator=(Segment&& s)
    {
        if(this->data != nullptr) {
            operator delete(this->data);
        }
        this->data = s.data;
        this->size = s.size;
        this->fill = s.fill;

        s.data = nullptr;

        return *this;
    }

    template<class T>
    void Heap<T>::set_seg_fill()
    {
        if(this->crr_seg != nullptr) {
            this->crr_seg->fill = this->crr_seg->size - this->mem_left;
        }
    }

    template<class T>
    T* Heap<T>::get_mem(size_t n_elems)
    {
        if(this->mem_left < n_elems) {
            this->alloc(n_elems);
        }

        this->mem_left -= n_elems;
        this->mem += n_elems;

        return this->mem - n_elems;
    }

    template<class T>
    void Heap<T>::clear()
    {
        this->segments.clear();
        this->mem_left = 0;
        this->cap = 0;
    }

    template<class T>
    void __attribute__ ((noinline)) Heap<T>::alloc(size_t n)
    {
        this->set_seg_fill();
        this->cap = cap < 1 ? 1 : cap * 2 < Max_Cap_Size ? cap * 2 : Max_Cap_Size;

        if(n < Max_Cap_Size && this->cap < n) {
            while(this->cap < n && this->cap < Max_Cap_Size) {
                this->cap *= 2;
            }
        }

        auto alloc_sz = this->cap > n ? this->cap : n;

        this->segments.emplace_back(alloc_sz);

        this->crr_seg = &this->segments.back();
        this->crr_seg->data = static_cast<T*>(operator new(sizeof(T) * alloc_sz));

        this->mem = this->crr_seg->data;
        this->mem_left = alloc_sz;
    }

    inline MemBlock copy(Heap<uint8_t>& heap, const MemBlock& block)
    {
        if(block.size < 1) {
            return block;
        }
        auto* mem = heap.get_mem(block.size);
        memcpy(mem, block.ptr, block.size);

        return { mem, block.size };
    }

} }

#endif
