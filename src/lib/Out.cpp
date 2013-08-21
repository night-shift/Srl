#include "Srl/Out.h"
#include "Srl/Lib.h"

using namespace std;
using namespace Srl;
using namespace Lib;

void Out::set(Source source)
{
    this->heap.clear();
    this->state = State();

    this->streaming = source.is_stream;
    this->stream = source.is_stream ? source.stream : nullptr;
    this->buffer = !source.is_stream ? source.buffer : nullptr;
}

void Out::inc_cap(size_t nbytes)
{
    this->state.cap = Stream_Buffer_Size < nbytes ? nbytes : Stream_Buffer_Size;
    this->state.mem_start = this->heap.get_mem(this->state.cap);
    this->state.left = this->state.cap;
    this->state.crr_mem = this->state.mem_start;
}

void Out::flush()
{
    if(this->streaming) {
        this->write_to_stream();
        this->stream->flush();

    } else {
        this->write_to_buffer();
    }
}

void Out::write_to_stream()
{
    if(!this->streaming || this->state.mem_start == nullptr) {
        return;
    }

    this->stream->write((const char*)this->state.mem_start, this->state.cap - this->state.left);

    this->state.left = this->state.cap;
    this->state.crr_mem = this->state.mem_start;
    this->state.segs_flushed++;
}

void Out::write_to_buffer()
{
    assert(this->buffer);

    this->buffer->resize(this->state.sz_total);

    auto pos = 0U;
    auto* seg = this->heap.chain.used_segs.front;

    while(seg) {
        auto fill = seg->val.size - seg->val.left;
        memcpy(buffer->data() + pos, seg->val.data, fill);
        pos += fill;
        seg = seg->next;
    }
}

