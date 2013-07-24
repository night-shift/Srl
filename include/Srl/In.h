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
        typedef std::function<void()>  OutOfBounds;
        typedef std::function<size_t(In&, std::vector<uint8_t>&, size_t idx)> Substitute;

        In(const uint8_t* data, size_t data_size)
            :  start(data), end(data + data_size + 1), current_pos(data)  { }

        In(std::istream& stream_);

        inline bool           is_streaming() const;
        inline const uint8_t* pointer()      const;

        inline void move           (size_t steps, const OutOfBounds& out_of_bounds);
        inline const uint8_t* peek (size_t steps, const OutOfBounds& out_of_bounds);
        inline bool try_peek       (size_t steps);
        inline MemBlock read_block (size_t steps, const OutOfBounds& out_of_bounds);

        template<class T>
        T cast_move (const OutOfBounds& out_of_bounds);

        template<class... Tokens>
        size_t move_until (const OutOfBounds& out_of_bounds, const Tokens&... tokens);

        template<class... Tokens>
        MemBlock read_block_until (const OutOfBounds& out_of_bounds, const Tokens&... tokens);

        template<class... Tokens>
        size_t read_substitue (const OutOfBounds& out_of_bounds, uint8_t delimiter,
                               std::vector<uint8_t>& buffer, const Tokens&... tokens);

        template<class... Tokens>
        size_t move_while (const OutOfBounds& out_of_bounds, const Tokens&... tokens);

        inline void skip_space(const OutOfBounds& out_of_bounds);

        template<size_t N = 1, class... Tokens>
        bool is_at_token (const char head, const Tokens&... tokens);

        template<size_t N, class... Tokens>
        bool is_at_token (const std::array<const char, N>& head, const Tokens&... tokens);

        inline bool is_at_token();

    private :
        static const size_t Stream_Buffer_Size = 8192;

        const uint8_t* start       = nullptr;
        const uint8_t* end         = nullptr;
        const uint8_t* current_pos = nullptr;

        bool streaming       = false;
        std::istream* stream = nullptr;
        bool eof_reached     = false;

        int buffer_acc = 0;
        std::vector<uint8_t> buffers[2];

        const uint8_t* buffer_anchor = nullptr;


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
        size_t move_until (const OutOfBounds& out_of_bounds, const Tokens&... tokens);

        bool try_fetch_data(size_t nbytes);
        void fetch_data(size_t nbytes, const OutOfBounds& out_of_bounds);
    };

} }

#endif
