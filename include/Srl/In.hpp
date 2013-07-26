#ifndef SRL_IN_HPP
#define SRL_IN_HPP

#include "In.h"
#include "Aux.h"

namespace Srl { namespace Lib {

    template<class T>
    T In::cast_move(const OutOfBounds& out_of_bounds)
    {
        auto block = this->read_block(sizeof(T), out_of_bounds);

        if(block.size < sizeof(T)) {
            return T();
        }

        return Aux::Read_Cast<T>(block.ptr);
    }

    template<class... Tokens>
    size_t In::move_until(const OutOfBounds& out_of_bounds, const Tokens&... tokens)
    {
        return this->move_until<false>(out_of_bounds, tokens...);
    }

    template<class... Tokens>
    MemBlock In::read_block_until(const OutOfBounds& out_of_bounds, const Tokens&... tokens)
    {
        this->buffer_anchor = this->current_pos;
        auto steps = this->move_until(out_of_bounds, tokens...);
        this->buffer_anchor = nullptr;

        return { this->current_pos - steps, steps };
    }

    template<class... Tokens>
    size_t In::move_while(const OutOfBounds& out_of_bounds, const Tokens&... tokens)
    {
        return this->move_until<true>(out_of_bounds, tokens...);
    }

    template<bool Not, class... Tokens>
    size_t In::move_until(const OutOfBounds& out_of_bounds, const Tokens&... tokens)
    {
        auto steps = 0U;

        while(Not == this->is_at_token(tokens...)) {
            this->move(1, out_of_bounds);
            steps++;
        }

        return steps;
    }

    template<size_t N, class... Tail>
    bool In::is_at_token(const char head, const Tail&... tail)
    {
        return *this->current_pos == head || this->is_at_token(tail...);
    }

    template<size_t N, class... Tail>
    bool In::is_at_token(const std::array<const char, N>& head, const Tail&... tail)
    {
        return (try_peek(N) && Aux::comp<N>(this->current_pos, head.data())) || this->is_at_token(tail...);
    }

    inline bool In::is_at_token()
    {
        return false;
    }

    inline void In::move(size_t steps, const OutOfBounds& out_of_bounds)
    {
        this->current_pos = this->peek(steps, out_of_bounds) + steps;
    }

    inline const uint8_t* In::peek(size_t steps, const OutOfBounds& out_of_bounds)
    {
        if(this->current_pos + steps >= this->end) {
            this->fetch_data(steps, out_of_bounds);
        }
        return this->current_pos;
    }

    inline bool In::try_peek(size_t steps)
    {
        return this->current_pos + steps < this->end || this->try_fetch_data(steps);
    }

    inline MemBlock In::read_block(size_t steps, const OutOfBounds& out_of_bounds)
    {
        this->buffer_anchor = this->current_pos;
        this->move(steps, out_of_bounds);
        this->buffer_anchor = nullptr;

        return { this->current_pos - steps, steps };
    }

    inline void In::skip_space(const OutOfBounds& out_of_bounds)
    {
        this->move_while(out_of_bounds, ' ', '\n', '\t', '\r', '\b', '\f', '\b');
    }

    template<class... Tokens>
    size_t In::read_substitue(const OutOfBounds& out_of_bounds, uint8_t delimiter,
                              std::vector<uint8_t>& buffer, const Tokens&... tokens)
    {
        const size_t max = Aux::max_len<Tokens...>();

        auto steps = 0U;
        auto buf_sz = buffer.size();

        while(*this->peek(0, out_of_bounds) != delimiter) {

            if(steps + 1 > buf_sz) {
                buf_sz += steps + 10;
                buffer.resize(buf_sz);
            }

            auto left = this->try_peek(max) ? max : this->end - this->current_pos - 1;
            steps += this->substitute_token(buffer, steps, left, tokens...);
        }

        return steps;
    }

    template<class Sub, class Token, class... Tail>
    size_t In::substitute_token(std::vector<uint8_t>& buffer, size_t idx, size_t left,
                                const Sub& sub, const Token& token, const Tail&... tail)
    {
        const auto N = std::tuple_size<Token>::value;

        return left >= N && Aux::comp<N>(this->current_pos, token.data())
                ? this->replace<Sub, N>(sub, buffer, idx)
                : this->substitute_token(buffer, idx, left, tail...);
    }

    inline size_t In::substitute_token(std::vector<uint8_t>& buffer, size_t idx, size_t)
    {
        buffer[idx] = *this->current_pos++;
        return 1;
    }

    template<class Sub, size_t N>
    typename std::enable_if<!std::is_same<Sub, In::Substitute>::value, size_t>::type
    In::replace(const Sub& substitute, std::vector<uint8_t>& buffer, size_t idx)
    {
        buffer[idx] = substitute;
        this->current_pos += N;

        return 1;
    }

    template<class Sub, size_t N>
    typename std::enable_if<std::is_same<Sub, In::Substitute>::value, size_t>::type
    In::replace(const Sub& substitute, std::vector<uint8_t>& buffer, size_t idx)
    {
        return substitute(*this, buffer, idx);
    }

    inline const uint8_t* In::pointer() const
    {
        return this->current_pos;
    }

    inline bool In::is_streaming() const
    {
        return this->streaming;
    }
} }

#endif
