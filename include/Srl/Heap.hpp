#ifndef SRL_HEAP_HPP
#define SRL_HEAP_HPP

#include "Common.h"
#include "Heap.h"
#include "Aux.h"

namespace Srl { namespace Lib {

    template<class T>
    Heap<T>::Segment::Segment(size_t size_)
    {
        this->data = static_cast<T*>(operator new(sizeof(T) * size_));
        this->size = size_;
        this->left = size_;
    }

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
        this->left = s.left;
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
        this->left = s.left;

        s.data = nullptr;

        return *this;
    }

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

    template<class T> __attribute__ ((noinline))
    typename Heap<T>::Segment* Heap<T>::alloc(size_t n)
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
