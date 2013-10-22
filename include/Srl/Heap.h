#ifndef SRL_HEAP_H
#define SRL_HEAP_H

#include "Common.h"

namespace Srl { namespace Lib {

    namespace Aux {

        template<class T> struct SList {

            struct Link {
                Link* next = nullptr;
                T     val;
            };

            SList() { }

            Link* front = nullptr;
            Link* back  = nullptr;

            void  clear   ();
            void  append  (Link* link);
            void  prepend (Link* link);
            void  remove  (Link* prev, Link* link);

            template<class Predicate>
            Link* find_rm (const Predicate& predicate);
        };
    }

    class Heap {

    friend class Out;

    public:
        Heap() { }
        Heap(const Heap&) = default;

        Heap(Heap&& m) { *this = std::forward<Heap>(m); };
        Heap& operator= (Heap&& m);

        ~Heap();

        template<class T = uint8_t>
        T* get_mem (size_t n_elems);

        template<class T, class... Args>
        T* create (Args&&... args);

        void put_mem(uint8_t* mem, size_t sz);

        void clear();

        struct Segment {

            Segment () { }
            ~Segment();

            uint32_t left    = 0;
            uint32_t size    = 0;
            uint8_t* data    = nullptr;
            bool     sub_seg = false;

            void     dec (size_t n) { this->left -= n; }
            uint8_t* pointer()      { return data + size - left; }
        };

    private:
        static const size_t Max_Cap = 32768;

        size_t   cap     = 256;
        Segment* crr_seg = nullptr;

        struct Chain {

            Aux::SList<Segment> used_segs;
            Aux::SList<Segment> free_segs;

            Aux::SList<Aux::SList<Segment>::Link> used_links;
            Aux::SList<Aux::SList<Segment>::Link> free_links;

            Aux::SList<Segment>::Link* get_link();

        } chain;

        Segment* alloc         (size_t sz);
        Segment* find_free_seg (size_t sz);

        void destroy();
    };

    template<class T>
    struct HeapAllocator {

        typedef T*                pointer;
        typedef const T*          const_pointer;
        typedef T                 value_type;
        typedef value_type&       reference;
        typedef const value_type& const_reference;

        HeapAllocator(Heap& heap_) : heap(&heap_) { }

        HeapAllocator(const HeapAllocator& a) : heap(a.heap) { }

        template<class U>
        HeapAllocator(const HeapAllocator<U>& a) : heap(a.heap) { }

        Heap* heap;

        pointer allocate(size_t n)
        {
            return this->heap->template get_mem<T>(n);
        }

        void deallocate(pointer mem, size_t n)
        {
            this->heap->template put_mem((uint8_t*)mem, n * sizeof(T));
        }

        void destroy(T* t)
        {
            t->~T();
        }

        template<class... Args>
        void construct(T* mem, const Args&... args)
        {
            new (mem) T { args... };
        }

        void construct(T* mem, const T& t)
        {
            new (mem) T { t };
        }

        template<typename U>
        struct rebind {
            typedef HeapAllocator<U> other;
        };
    };

} }

#include "Heap.hpp"

#endif
