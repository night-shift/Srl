#ifndef SRL_HEAP_H
#define SRL_HEAP_H

#include "Common.h"

namespace Srl { namespace Lib {

    template<class T>
    class Heap {

    struct Segment;
    friend class Out;

    public:
        
        Heap() = default;
        Heap(const Heap&) = default;

        Heap(Heap&& m) { *this = std::forward<Heap>(m); };
        Heap& operator= (Heap&& m);

        T* get_mem (size_t n_elems);
        template<class... Args>
        T* create (const Args&... args);
        void clear ();

    private:
        static const size_t Max_Cap_Size  = 262144 / sizeof(T);

        size_t   cap     = 0;
        Segment* crr_seg = nullptr;

        std::list<Segment> segments;

        Segment* alloc (size_t n_elems);

        struct Segment {

            Segment (size_t size_);
            ~Segment();
            Segment (Segment&& s)      { *this = std::forward<Segment>(s); }
            Segment (const Segment& s) { *this = s; }

            Segment& operator= (Segment&&);
            Segment& operator= (const Segment&);

            uint32_t left = 0;
            uint32_t size;
            T*       data = nullptr;
        };

    };

    template<class T>
    Heap<T>& Heap<T>::operator= (Heap<T>&& m)
    {
        this->clear();
        this->cap = m.cap;
        this->crr_seg = m.crr_seg;
        this->segments = std::move(m.segments);
        m.clear();

        return *this;
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
    Heap<T>::Segment::Segment(size_t size_)
    {
        this->data = static_cast<T*>(operator new(sizeof(T) * size_));
        this->size = size_;
        this->left = size_;
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
} }

#endif
