#include "Srl/In.h"
#include "Srl/Internal.h"

using namespace Srl;
using namespace Lib;

In::In(std::istream& stream_) : streaming(true), stream(&stream_)
{
    this->fetch_data(0);
}

void In::fetch_data(size_t nbytes, const std::function<void()>& out_of_bounds)
{
    if(!this->streaming || !this->fetch_data(nbytes)) {
        out_of_bounds();
    }
}

bool In::fetch_data(size_t nbytes)
{
    /*           (   <--------->   ) -> this needs to be preserved
     * [.........*.............*...] -> buffer
     *           |             |->  current pos
     *           |
     *           |-> possible buffer mark
     *
     * after reading new chunk:
     *
     * ( <- preserved -> )(      <- new data ->      )
     * [*.............*..............................]
     *  |             |-> new current pos
     *  |
     *  |-> new buffer mark
     */

    if(this->eof_reached || !this->streaming) {
        return false;
    }

    auto left = this->end - this->current_pos;

    auto read_len = (nbytes < Stream_Buffer_Size ? Stream_Buffer_Size : nbytes) - left;


    auto preserve_pos = this->buffer_mark != nullptr
        ? this->buffer_mark - start
        : (this->end - start) - left;

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
        memcpy(&buffer[0], &buffer_prev[preserve_pos - margin], preserve_sz);
    }

    this->stream->read((char*)&buffer[preserve_sz], read_len);
    auto bytes_read = (size_t)this->stream->gcount();

    this->current_pos = &buffer[preserve_sz - left];
    buffer_mark  = buffer_mark == nullptr ? nullptr : &buffer[margin];

    this->start = &buffer[0];
    this->end   = &buffer[0] + bytes_read + preserve_sz;

    if(bytes_read + left < nbytes) {

        assert(this->stream->eof());
        this->eof_reached = true;

        return false;
    }

    return true;
}

