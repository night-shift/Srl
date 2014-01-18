#include "Srl/Heap.h"

using namespace std;
using namespace Srl;
using namespace Lib;

namespace {

    using SegLink   = Aux::SList<Heap::Segment>::Link;
    using ChainLink = Aux::SList<SegLink>::Link;

    void set_subsegment(Heap::Segment& seg, uint8_t* data, size_t sz)
    {
        seg.data = data;
        seg.left = sz;
        seg.size = sz;
        seg.sub_seg = true;
    }

    void set_segment(Heap::Segment& seg, size_t sz)
    {
        seg.data = new uint8_t[sz];
        seg.left = sz;
        seg.size = sz;
        seg.sub_seg = false;
    }

    void reset_segment(SegLink& link)
    {
        link.val.left = link.val.size;
        link.next = nullptr;
    }

    struct Any {
        template<class T>
        bool operator() (const T&) const { return true; }
    };

    struct Size {
        Size(size_t size_, size_t align_) : size(size_), align(align_) { }

        const size_t size;
        const size_t align;

        template<class T>
        bool operator() (const T& t) const
        {
            assert(align > 0);

            return t.size >= size + align - 1 || (t.size >= size && (size_t)t.data % align == 0);
        }
    };
}

template<class T>
void Aux::SList<T>::clear()
{
    this->front = nullptr;
    this->back  = nullptr;
}

template<class T>
void Aux::SList<T>::append(SList<T>::Link* link)
{
    if(!this->back) {
        this->front = link;
        this->back  = link;

    } else {
        this->back->next = link;
        this->back = link;
    }

    link->next = nullptr;
}

template<class T> template<class Pred>
void Aux::SList<T>::prepend(const Pred& pred, SList<T>::Link* link)
{
    if(!this->back) {
        this->front = link;
        this->back  = link;

    } else {

        Link* curr = this->front;
        Link* prev = nullptr;

        while(curr) {
            if(pred(curr->val)) {
                link->next = curr;
                if(prev) {
                    prev->next = link;
                } else {
                    front = link;
                }
                return;
            }
            prev = curr;
            curr = curr->next;
        }

        link->next = nullptr;
        this->back->next = link;
        this->back = link;

    }
}

template<class T>
void Aux::SList<T>::remove(SList<T>::Link* prev, SList<T>::Link* link)
{
    this->front = link == front ? link->next : front;
    this->back  = link == back ? prev : back;
    if(prev) {
        prev->next = link->next;
    }
    link->next = nullptr;
}

template<class T>
void Aux::SList<T>::prepend(SList<T>::Link* link)
{
    if(!this->front) {
        this->append(link);

    } else {
        link->next = this->front;
        this->front = link;
    }
}

template<class T> template<class Predicate> typename Aux::SList<T>::Link*
Aux::SList<T>::find_rm(const Predicate& pred)
{
    Link* link = this->front;
    Link* prev = nullptr;

    while(link) {
        if(pred(link->val)) {
            remove(prev, link);
            return link;
        }

        prev = link;
        link = link->next;
    }

    return nullptr;
}

void Heap::put_mem(uint8_t* mem, size_t sz)
{

    auto* link = this->chain.get_link();
    set_subsegment(link->val, mem, sz);

    this->chain.free_segs.append(link);
}

Heap::Segment* Heap::find_free_seg(size_t sz, size_t align)
{
    auto* link = this->chain.free_segs.find_rm(Size(sz, align));

    if(link) {
        this->chain.used_segs.append(link);
    }
    return link ? &link->val : nullptr;
}

SegLink* Heap::Chain::get_link()
{
    auto* link = this->free_links.find_rm(Any());

    if(!link) {
        link = new ChainLink();
    }

    this->used_links.append(link);

    return &link->val;
}

Heap::Segment* Heap::alloc(size_t n, size_t align)
{
    Segment* seg = nullptr;

    if(this->crr_seg && crr_seg->left > 0) {
        this->put_mem(crr_seg->pointer(), this->crr_seg->left);
    }

    if(chain.free_segs.front && (seg = find_free_seg(n, align))) {
        return seg;
    }

    this->cap = cap < 1 ? 1 : cap * 2 < Max_Cap ? cap * 2 : Max_Cap;

    if(n < Max_Cap && this->cap < n) {
        while(this->cap < n && this->cap < Max_Cap) {
            this->cap *= 2;
        }
    }

    auto alloc_sz = this->cap > n ? this->cap : n;

    auto* link = this->chain.get_link();
    seg = &link->val;
    set_segment(*seg, alloc_sz);

    this->chain.used_segs.append(link);

    return seg;
}

void Heap::clear()
{
    this->chain.used_segs.clear();
    this->chain.free_segs.clear();

    ChainLink* link = this->chain.used_links.front;
    ChainLink* prev = nullptr;

    while(link) {
        auto* next = link->next;

        if(link->val.val.sub_seg) {
            this->chain.used_links.remove(prev, link);
            this->chain.free_links.append(link);

        } else {
            prev = link;
            reset_segment(link->val);
            this->chain.free_segs.prepend(Size(link->val.val.size, 1), &link->val);
        }

        link = next;
    }

    this->crr_seg = nullptr;
}

Heap::~Heap()
{
    this->destroy();
}

void Heap::destroy()
{
    const auto free = [](Aux::SList<SegLink>& lst) {
        auto* link = lst.front;
        while(link) {
            auto* next = link->next;
            delete link;
            link = next;
        }
    };

    free(this->chain.used_links);
    free(this->chain.free_links);

    this->chain = Chain();
    this->crr_seg = nullptr;
}

Heap& Heap::operator= (Heap&& m)
{
    this->destroy();

    this->cap     = m.cap;
    this->crr_seg = m.crr_seg;
    this->chain   = m.chain;

    m.chain = Chain();
    m.crr_seg = nullptr;

    return *this;
}

Heap::Segment::~Segment()
{
    if(!this->sub_seg && this->data) {
        delete[] this->data;
        this->data = nullptr;
    }
}

