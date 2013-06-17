#ifndef SRL_HEAP_H
#define SRL_HEAP_H

#include "Common.h"
#include "Blocks.h"

namespace Srl { namespace Lib {

    template<class T>
    class Heap {

    struct Segment;
    friend class Out;

    public:
        T* get_mem (size_t n_elems);
        void clear ();

    private:
        static const size_t Max_Cap_Size  = 524288 / sizeof(T);

        size_t   cap     = 0;
        Segment* crr_seg = nullptr;

        std::deque<Segment> segments;

        Segment* alloc (size_t n_elems);

        struct Segment {

            Segment(size_t size_);
            ~Segment();
            Segment (Segment&& s)      { *this = std::move(s); }
            Segment (const Segment& s) { *this = s; }

            Segment& operator= (Segment&&);
            Segment& operator= (const Segment&);

            size_t  left = 0;
            T*      data = nullptr;
            size_t  size;
        };

    };

    inline MemBlock copy_block(Heap<uint8_t>& heap, const MemBlock& block);

} }

#include "Heap.hpp"

#endif
