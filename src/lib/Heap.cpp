#include "Srl/Heap.h"

using namespace std;
using namespace Srl;
using namespace Lib;

namespace {

    const size_t recycle_thld = 256;

    using SegLink   = Heap::SList<Heap::Segment>::Link;
    using ChainLink = Heap::SList<SegLink>::Link;

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

    void reset_segment(Heap::Segment& seg)
    {
        seg.left = seg.size;
    }

    struct Any {
        template<class T>
        bool operator() (const T&) const { return true; }
    };

    struct Size {
        Size(size_t size_) : size(size_) { }

        const size_t size;

        template<class T>
        bool operator() (const T& t) const { return t.size >= size; }
    };
}

template<class T>
void Heap::SList<T>::clear()
{
    this->front = nullptr;
    this->back  = nullptr;
}

template<class T>
void Heap::SList<T>::append(SList<T>::Link* link)
{
    if(!this->back) {
        this->front = link;
        this->back  = link;

    } else {
        this->back->next = link;
        this->back = link;
    }
}

template<class T>
void Heap::SList<T>::remove(SList<T>::Link* prev, SList<T>::Link* link)
{
    this->front = link == front ? link->next : front;
    this->back  = link == back ? prev : back;
    if(prev) {
        prev->next = link->next;
    }
    link->next = nullptr;
}

template<class T>
void Heap::SList<T>::prepend(SList<T>::Link* link)
{
    if(!this->front) {
        this->append(link);

    } else {
        link->next = this->front;
        this->front = link;
    }
}

template<class T> template<class Predicate> typename Heap::SList<T>::Link*
Heap::SList<T>::find_rm(const Predicate& predicate)
{
    Link* link = this->front;
    Link* prev = nullptr;

    while(link) {
        if(predicate(link->val)) {
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
    if(sz < recycle_thld) {
        return;
    }

    auto* link = get_link();
    set_subsegment(link->val, mem, sz);

    this->chain.free_segs.prepend(link);
}

Heap::Segment* Heap::find_free_seg(size_t sz)
{
    auto* link = this->chain.free_segs.find_rm(Size(sz));

    if(link) {
        this->chain.used_segs.append(link);
    }
    return link ? &link->val : nullptr;
}

SegLink* Heap::get_link()
{
    auto* link = this->chain.free_links.find_rm(Any());

    if(!link) {
        link = new ChainLink();
    }

    this->chain.used_links.append(link);

    return &link->val;
}

Heap::Segment* Heap::alloc(size_t n)
{
    Segment* seg = nullptr;

    if(this->crr_seg && crr_seg->left > recycle_thld) {
        this->put_mem(crr_seg->pointer(), this->crr_seg->left);

    } else if(chain.free_segs.front && (seg = find_free_seg(n))) {
        return seg;
    }

    this->cap = cap < 1 ? 1 : cap * 2 < Max_Cap ? cap * 2 : Max_Cap;

    if(n < Max_Cap && this->cap < n) {
        while(this->cap < n && this->cap < Max_Cap) {
            this->cap *= 2;
        }
    }

    auto alloc_sz = this->cap > n ? this->cap : n;

    auto* link = this->get_link();
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
            reset_segment(link->val.val);
            this->chain.free_segs.prepend(&link->val);
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
    const auto free = [](SList<SegLink>& lst) {
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

