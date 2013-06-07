#include "Srl/Heap.h"

using namespace Srl;
using namespace Lib;

void Heap::alloc(size_t nbytes)
{
    auto new_cap = this->cap < Init_Segment_size ? Init_Segment_size
        : this->cap * 2 > this->max_cap
            ? this->max_cap
            : this->cap * 2;

    new_cap = new_cap < nbytes ? nbytes : new_cap;

    this->segments.emplace_back(new_cap);

    this->set_seg_fill();

    this->crr_seg = &this->segments.back();
    this->crr_seg->data = new uint8_t[new_cap];

    this->mem = this->crr_seg->data;
    this->cap = new_cap;
    this->mem_left = new_cap;
}
