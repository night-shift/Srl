#ifndef SRL_OUT_H
#define SRL_OUT_H

#include "Heap.h"
#include "String.h"

#include <ostream>

namespace Srl { namespace Lib {

    class Out {

    public:
        Out ()
            : streaming(false), stream(nullptr) { }

        Out (std::ostream& stream_, size_t buffer_size = Max_Stream_Buffer_size)
            : max_cap(buffer_size), streaming(true), stream(&stream_) { }

        class Ticket;

        inline void  write (uint8_t byte);
        inline void  write (const MemBlock& block);
        inline void  write (const uint8_t* bytes, size_t nbytes);
        inline void  write (Ticket& ticket, const uint8_t* bytes, size_t offset, size_t nbytes);
        inline void  write_times (size_t n_times, uint8_t byte);

        template<class... Tokens>
        void write_substitute(const Lib::MemBlock& data, const Tokens&... tokens);

        void            flush();
        inline Ticket   reserve (size_t nbytes);

        std::vector<uint8_t> extract();

        class Ticket {

        friend class Out;

        private:
            Ticket(uint8_t* mem_, size_t size_, size_t pos_, size_t seg_id_)
                : mem(mem_), size(size_), pos(pos_), seg_id(seg_id_) { }

            uint8_t* mem;
            size_t   size;
            size_t   pos;
            size_t   seg_id;
        };

    private:
        static const size_t Max_Stream_Buffer_size = 65536;

        Heap buffer;

        size_t sz_total     = 0;
        size_t left         = 0;
        size_t cap          = 0;
        size_t max_cap      = 0;
        size_t segs_flushed = 0;

        uint8_t* crr_mem   = nullptr;
        uint8_t* mem_start = nullptr;

        bool streaming;
        std::ostream* stream;

        void inc_cap          (size_t nbytes);
        inline uint8_t* alloc (size_t nbytes);

        template<class Token, class Sub, class... Tokens>
        void substitute_token(uint8_t src, const Token& token, const Sub& sub, const Tokens&... tail);

        inline void substitute_token(uint8_t src);

        template<size_t N, class TSub>
        typename std::enable_if<N != 0, void>::type
        write_substitute(uint8_t* dest, const TSub* sub);

        template<size_t N, class TSub>
        typename std::enable_if<N == 0, void>::type
        write_substitute(uint8_t*, const TSub*) { }
    };
} }

#endif
