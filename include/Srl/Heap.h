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

        void clear();

    private:
        static const size_t Max_Cap_Size  = 524288 / sizeof(T);

        size_t   cap             = 0;
        size_t   mem_left        = 0;
        T*       mem             = nullptr;
        Segment* crr_seg         = nullptr;

        std::deque<Segment> segments;

        void alloc        (size_t n_elems);
        void set_seg_fill ();

        struct Segment {

            Segment(size_t size_) : size(size_) { }
            ~Segment();
            Segment (Segment&& s)      { *this = std::move(s); }
            Segment (const Segment& s) { *this = s; }

            Segment& operator= (Segment&&);
            Segment& operator= (const Segment&);

            size_t  fill = 0;
            T*      data = nullptr;
            size_t  size;
        };

    };

} }

#include "Heap.hpp"

#endif
