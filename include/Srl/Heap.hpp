#ifndef SRL_HEAP_HPP
#define SRL_HEAP_HPP

#include "Common.h"
#include "Heap.h"
#include "Aux.h"

namespace Srl { namespace Lib {

    template<class T>
    T* Heap<T>::get_mem(size_t n_elems)
    {
        if(srl_unlikely(!this->crr_seg || this->crr_seg->left < n_elems)) {
            this->crr_seg = this->alloc(n_elems);
        }

        this->crr_seg->left -= n_elems;

        return this->crr_seg->data + this->crr_seg->size - (this->crr_seg->left + n_elems);
    }

    template<class T>
    template<class... Args>
    T* Heap<T>::create (const Args&... args)
    {
        auto* mem = get_mem(1);
        return new (mem) T { args... };
    }

    template<class T>
    void Heap<T>::clear()
    {
        this->segments.clear();
        this->cap = 0;
        this->crr_seg = nullptr;
    }

    template<class T>
    srl_noinline typename Heap<T>::Segment* Heap<T>::alloc(size_t n)
    {
        this->cap = cap < 1 ? 1 : cap * 2 < Max_Cap_Size ? cap * 2 : Max_Cap_Size;

        if(n < Max_Cap_Size && this->cap < n) {
            while(this->cap < n && this->cap < Max_Cap_Size) {
                this->cap *= 2;
            }
        }

        auto alloc_sz = this->cap > n ? this->cap : n;
        this->segments.emplace_back(alloc_sz);

        return &this->segments.back();
    }

} }

#endif
