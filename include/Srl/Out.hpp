#ifndef SRL_OUT_HPP
#define SRL_OUT_HPP

#include "Out.h"
#include "Aux.h"

namespace Srl { namespace Lib {

    inline uint8_t* Out::alloc(size_t nbytes)
    {
        this->state.sz_total += nbytes;

        if(!this->streaming) {
            return this->heap.get_mem(nbytes);
        }

        if(this->state.left < nbytes) {
            this->write_to_stream();
        }

        if(this->state.cap < nbytes) {
            this->inc_cap(nbytes);
        }

        this->state.left -= nbytes;
        this->state.crr_mem += nbytes;

        return this->state.crr_mem - nbytes;
    }

    inline void Out::write(const uint8_t* bytes, size_t nbytes)
    {
        auto* mem = this->alloc(nbytes);

        if(nbytes == 1) {
            *mem = *bytes;
        } else {
            memcpy(mem, bytes, nbytes);
        }
    }

    inline void Out::write(Out::Ticket& ticket, const uint8_t* bytes, size_t offset, size_t nbytes)
    {
        assert(ticket.size >= offset + nbytes);

        if(!this->streaming || ticket.seg_id == this->state.segs_flushed) {
            memcpy(ticket.mem + offset, bytes, nbytes);

        } else {
            auto crr = this->stream->tellp();

            this->stream->seekp(ticket.pos + offset);
            this->stream->write((const char*)bytes, nbytes);

            this->stream->seekp(crr);
        }
    }

    inline Out::Ticket Out::reserve(size_t nbytes)
    {
        auto pos = this->state.sz_total;
        auto* reserved = this->alloc(nbytes);
        memset(reserved, 0, nbytes);

        return { reserved, nbytes, pos, this->state.segs_flushed };
    }

    inline void Out::write_byte(uint8_t byte)
    {
        *this->alloc(1) = byte;
    }

    template<class T> void Out::write(const T& o)
    {
        auto* mem = this->alloc(sizeof(T));
        Aux::copy<sizeof(T)>((const uint8_t*)&o, mem);
    }

    inline void Out::write(const MemBlock& block)
    {
        this->write(block.ptr, block.size);
    }

    inline void Out::write_times(size_t n_times, uint8_t byte)
    {
        auto* mem = this->alloc(n_times);
        memset(mem, byte, n_times);
    }

    template<class... Tokens>
    void Out::write_substitute(const Lib::MemBlock& data, const Tokens&... tokens)
    {
        for(auto i = 0U; i < data.size; i++) {
            this->substitute_token(*(data.ptr + i), tokens...);
        }
    }

    template<class Token, class Sub, class... Tokens>
    void Out::substitute_token(uint8_t src, const Token& token, const Sub& sub, const Tokens&... tail)
    {
        if(src != token) {
            this->substitute_token(src, tail...);

        } else {
            auto* dest = this->alloc(std::tuple_size<Sub>::value);
            this->write_substitute<std::tuple_size<Sub>::value>(dest, sub.data());
        }
    }

    inline void Out::substitute_token(uint8_t src) { this->write(src); }

    template<size_t N, class Sub> typename std::enable_if<N != 0, void>::type
    Out::write_substitute(uint8_t* dest, const Sub* sub)
    {
        *dest = *sub;
        write_substitute<N - 1>(dest + 1, sub + 1);
    }

} }

#endif
