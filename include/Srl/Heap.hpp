#ifndef SRL_HEAP_HPP
#define SRL_HEAP_HPP

#include "Common.h"
#include "Heap.h"
#include "Aux.h"

namespace Srl { namespace Lib {

    template<class T>
    T* Heap::get_mem(size_t n_elems)
    {
        const auto etra_alignment = alignof(T) - 1;

        auto sz = n_elems * sizeof(T);

        if(srl_unlikely(!this->crr_seg || this->crr_seg->left < sz + etra_alignment)) {
            this->crr_seg = this->alloc(sz + etra_alignment);
        }

        auto* mem = this->crr_seg->pointer();

        auto rem    = (size_t)mem % alignof(T);
        auto offset = rem ? alignof(T) - rem : 0;

        mem += offset;
        this->crr_seg->dec(sz + offset);

        return reinterpret_cast<T*>(mem);
    }

    template<class T, class... Args>
    T* Heap::create (const Args&... args)
    {
        auto* mem = get_mem<T>(1);

        return new (mem) T { args... };
    }

} }

#endif
