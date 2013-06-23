#include "Srl/In.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

In::In(istream& stream_) : streaming(true), stream(&stream_)
{
    this->try_fetch_data(0);
}

void In::fetch_data(size_t nbytes, const OutOfBounds& out_of_bounds)
{
    if(!this->streaming || !this->try_fetch_data(nbytes)) {
        out_of_bounds();
    }
}

bool In::try_fetch_data(size_t nbytes)
{
    /*           (   <--------->   ) -> this needs to be preserved
     * [.........*.............*...] -> buffer
     *           |             |->  current pos
     *           |
     *           |-> possible buffer anchor
     *
     * after reading new chunk:
     *
     * ( <- preserved -> )(      <- new data ->      )
     * [*.............*..............................]
     *  |             |-> new current pos
     *  |
     *  |-> new buffer anchor
     */

    if(this->eof_reached || !this->streaming) {
        return false;
    }

    auto left = this->end - this->current_pos;

    auto read_len = (nbytes < Stream_Buffer_Size ? Stream_Buffer_Size : nbytes) - left;


    auto preserve_pos = this->buffer_anchor != nullptr
        ? this->buffer_anchor - start
        : (this->end - start) - left;

    /* small extra margin to make sure memory around the current position will be preserved in any case */
    const auto margin_sz = 32;
    auto margin = preserve_pos < margin_sz ? margin_sz - (margin_sz - preserve_pos) : margin_sz;

    auto preserve_sz = this->end - (this->start + preserve_pos - margin);
    auto n_buffer_sz = preserve_sz + read_len;

    auto& buffer_prev = this->buffers[this->buffer_acc];
    this->buffer_acc = (this->buffer_acc + 1) % 2;
    auto& buffer = this->buffers[this->buffer_acc];

    if(buffer.size() < n_buffer_sz) {
        buffer.resize(n_buffer_sz);
    }

    if(preserve_sz > 0) {
        memcpy(buffer.data(), &buffer_prev[preserve_pos - margin], preserve_sz);
    }

    this->stream->read((char*)&buffer[preserve_sz], read_len);
    auto bytes_read = (size_t)this->stream->gcount();

    this->current_pos = &buffer[preserve_sz - left];
    buffer_anchor  = buffer_anchor == nullptr ? nullptr : &buffer[margin];

    this->start = buffer.data();
    this->end   = buffer.data() + bytes_read + preserve_sz;

    if(bytes_read + left < nbytes) {

        assert(this->stream->eof());
        this->eof_reached = true;

        return false;
    }

    return true;
}
