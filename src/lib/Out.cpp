#include "Srl/Out.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

void Out::inc_cap(size_t nbytes)
{
    this->cap = Stream_Buffer_Size < nbytes ? nbytes : Stream_Buffer_Size;
    this->mem_start = this->buffer.get_mem(this->cap);
    this->left = this->cap;
    this->crr_mem = this->mem_start;
}

void Out::flush()
{
    if(this->streaming) {
        this->write_to_stream();
        this->stream->flush();
    }
}

void Out::write_to_stream()
{
    if(!this->streaming || this->mem_start == nullptr) {
        return;
    }

    this->stream->write((const char*)this->mem_start, this->cap - this->left);

    this->left = this->cap;
    this->crr_mem = this->mem_start;
    this->segs_flushed++;
}

vector<uint8_t> Out::extract()
{
    vector<uint8_t> vec(this->sz_total);

    auto pos = 0U;
    for(auto& s : this->buffer.segments) {
        auto fill = s.size - s.left;
        memcpy(&vec[pos], s.data, fill);
        pos += fill;
    }

    this->buffer.clear();

    return move(vec);
}

