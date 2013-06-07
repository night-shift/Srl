#include "Srl/Out.h"

using namespace Srl;
using namespace Lib;

void Out::inc_cap(size_t nbytes)
{
    auto alloc_sz = this->max_cap < nbytes ? nbytes : max_cap;
    this->mem_start = this->buffer.get_mem(alloc_sz);
    this->cap = alloc_sz;
    this->left = alloc_sz;
    this->crr_mem = this->mem_start;
}

void Out::flush()
{
    if(!this->streaming || this->mem_start == nullptr) {
        return;
    }

    this->stream->write((const char*)this->mem_start, this->cap - this->left);
    this->stream->flush();
    this->left = this->cap;
    this->crr_mem = this->mem_start;
    this->segs_flushed++;
}

std::vector<uint8_t> Out::extract()
{
    this->buffer.set_seg_fill();

    std::vector<uint8_t> vec(this->sz_total);

    auto pos = 0U;
    for(auto& s : this->buffer.segments) {
        memcpy(&vec[pos], s.data, s.fill);
        pos += s.fill;
    }

    return std::move(vec);
}

