#ifndef SRL_HEAP_H
#define SRL_HEAP_H

#include "Common.h"
#include "Blocks.h"

namespace Srl { namespace Lib {

    class Heap {

    struct Segment;
    friend class Out;

    public:
        Heap (size_t max_segment_size = Max_Segment_size)
            : max_cap(max_segment_size) { }

        inline MemBlock copy       (const MemBlock& block);
        inline uint8_t* get_mem    (size_t nbytes);

        template<class T> uint8_t* alloc_type();
        inline void clear();

    private:
        static const size_t Max_Segment_size  = 1048576;
        static const size_t Init_Segment_size = 128;

        size_t max_cap;

        size_t   cap      = 0;
        size_t   mem_left = 0;
        uint8_t* mem      = nullptr;
        Segment* crr_seg  = nullptr;

        std::deque<Segment> segments;

        void alloc               (size_t nbytes);
        inline void set_seg_fill ();

        struct Segment {

            Segment(size_t size_) : size(size_) { }
            inline ~Segment();
            Segment (Segment&& s)      { *this = std::move(s); }
            Segment (const Segment& s) { *this = s; }

            inline Segment& operator= (Segment&&);
            inline Segment& operator= (const Segment&);

            size_t   fill = 0;
            uint8_t* data = nullptr;
            size_t   size;
        };

    };
} }

#include "Heap.hpp"

#endif
