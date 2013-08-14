#include "Srl/In.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

void In::set(Source source)
{
    this->streaming = source.is_stream;
    this->anchor = nullptr;
    this->eof_reached = false;

    if(source.is_stream) {
        this->stream = source.stream;
        this->start = this->end = this->pos = nullptr;
        this->try_fetch_data(0, 1);

    } else {
        this->start = this->pos = source.block.ptr;
        this->end   = this->start + source.block.size + 1;
    }
}

void In::fetch_data(size_t nbytes, const Error& error)
{
    if(!this->streaming || !this->try_fetch_data(nbytes)) {
        error();
    }
}

bool In::try_fetch_data(size_t nbytes, size_t read_len)
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

    auto left = this->end - this->pos;

    read_len = (nbytes < read_len ? read_len : nbytes) - left;

    auto preserve_pos = this->anchor != nullptr
        ? this->anchor - start
        : (this->end - start) - left;

    /* small extra margin to make sure memory around the current position will be preserved in any case */
    const auto margin_sz = 32;
    auto margin = preserve_pos < margin_sz ? margin_sz - (margin_sz - preserve_pos) : margin_sz;

    auto preserve_sz = this->end - (this->start + preserve_pos - margin);
    auto bufsz = preserve_sz + read_len;

    auto& bufprev = this->swap[this->swap_mod];

    this->swap_mod = (this->swap_mod + 1) % 2;
    auto& buf = this->swap[this->swap_mod];

    if(buf.size() < bufsz) {
        buf.resize(bufsz);
    }

    if(preserve_sz > 0) {
        memcpy(buf.data(), &bufprev[preserve_pos - margin], preserve_sz);
    }

    this->stream->read((char*)&buf[preserve_sz], read_len);
    auto bytes_read = (size_t)this->stream->gcount();

    this->pos  = &buf[preserve_sz - left];
    anchor = anchor ? nullptr : &buf[margin];

    this->start = buf.data();
    this->end   = buf.data() + bytes_read + preserve_sz;

    if(bytes_read + left < nbytes) {
        this->eof_reached = true;

        return false;
    }

    return true;
}
