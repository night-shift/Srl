#ifndef SRL_IN_H
#define SRL_IN_H

#include "Common.h"
#include "Blocks.h"
#include "String.h"

#include <functional>
#include <istream>

namespace Srl { namespace Lib {

    class In {

    public :
        struct Source {

            Source(const std::vector<uint8_t>& data) : Source(data.data(), data.size()) { }

            Source(const uint8_t* data, size_t size)
                : block({ data, size }), is_stream(false)  { }

            Source(std::istream& stream_)
                : stream(&stream_), is_stream(true) { }

            union {
                std::istream* stream;
                MemBlock      block;
            };
            bool is_stream;
        };

        typedef std::function<void()>  Error;
        typedef std::function<size_t(In&, std::vector<uint8_t>&, size_t idx)> Substitute;

        struct Notify {
            Notify(char token_, bool& note_) : token(token_), note(&note_) { }
            char  token;
            bool* note;
        };

        In() { }
        
        void                  set(Source source); 

        inline bool           is_streaming() const;
        inline const uint8_t* pointer()      const;

        inline void move           (size_t steps, const Error& error);
        inline const uint8_t* peek (size_t steps, const Error& error);
        inline bool     try_peek   (size_t steps);
        inline MemBlock read_block (size_t steps, const Error& error);

        template<class T>
        T read_move (const Error& error);

        template<class... Tokens>
        size_t move_until (const Error& error, const Tokens&... tokens);

        template<class... Tokens>
        MemBlock read_block_until (const Error& error, const Tokens&... tokens);

        template<class... Tokens>
        size_t read_substitue (const Error& error, uint8_t delimiter,
                               std::vector<uint8_t>& buffer, const Tokens&... tokens);

        template<class... Tokens>
        size_t move_while (const Error& error, const Tokens&... tokens);

        inline void skip_space(const Error& error);

        template<size_t N = 1, class... Tail>
        bool is_at_token (const char head, const Tail&... tail);

        template<size_t N, class... Tail>
        bool is_at_token (const std::array<const char, N>& head, const Tail&... tail);

        template<size_t N = 1, class... Tail>
        bool is_at_token(const Notify& head, const Tail&... tail);

        bool is_at_token() { return false; }

    private :
        const uint8_t* start = nullptr;
        const uint8_t* end   = nullptr;
        const uint8_t* pos   = nullptr;

        bool streaming       = false;
        std::istream* stream = nullptr;
        bool eof_reached     = false;

        int swap_mod = 0;
        std::vector<uint8_t> swap[2];

        const uint8_t* anchor = nullptr;

        template<class Sub, class Token, class... Tail>
        size_t substitute_token(std::vector<uint8_t>& buffer, size_t idx, size_t left, const Sub& sub,
                                const Token& token, const Tail&... tail);

        inline size_t substitute_token(std::vector<uint8_t>& buffer, size_t idx, size_t left);

        template<class Sub, size_t N>
        typename std::enable_if<!std::is_same<Sub, In::Substitute>::value, size_t>::type
        replace(const Sub& sub, std::vector<uint8_t>& buffer, size_t idx);

        template<class Sub, size_t N>
        typename std::enable_if<std::is_same<Sub, In::Substitute>::value, size_t>::type
        replace(const Sub& sub, std::vector<uint8_t>& buffer, size_t idx);

        template<bool Not, class... Tokens>
        size_t move_until (const Error& error, const Tokens&... tokens);

        bool try_fetch_data(size_t nbytes, size_t read_len = 1024);
        void fetch_data(size_t nbytes, const Error& error);
    };

} }

#endif
