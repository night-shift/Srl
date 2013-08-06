#ifndef SRL_IN_HPP
#define SRL_IN_HPP

#include "In.h"
#include "Aux.h"

namespace Srl { namespace Lib {

    template<class T>
    T In::read_move(const Error& error)
    {
        auto block = this->read_block(sizeof(T), error);

        if(block.size < sizeof(T)) {
            return T();
        }

        return Aux::read<T>(block.ptr);
    }

    template<class... Tokens>
    size_t In::move_until(const Error& error, const Tokens&... tokens)
    {
        return this->move_until<false>(error, tokens...);
    }

    template<class... Tokens>
    MemBlock In::read_block_until(const Error& error, const Tokens&... tokens)
    {
        this->anchor = this->pos;
        auto steps = this->move_until(error, tokens...);
        this->anchor = nullptr;

        return { this->pos - steps, steps };
    }

    template<class... Tokens>
    size_t In::move_while(const Error& error, const Tokens&... tokens)
    {
        return this->move_until<true>(error, tokens...);
    }

    template<bool Not, class... Tokens>
    size_t In::move_until(const Error& error, const Tokens&... tokens)
    {
        auto steps = 0U;

        while(Not == this->is_at_token(tokens...)) {
            this->move(1, error);
            steps++;
        }

        return steps;
    }

    template<size_t N, class... Tail>
    bool In::is_at_token(const char head, const Tail&... tail)
    {
        return *this->pos == head || this->is_at_token(tail...);
    }

    template<size_t N, class... Tail>
    bool In::is_at_token(const Notify& head, const Tail&... tail)
    {
        if(*this->pos == head.token) *head.note = true;
        return this->is_at_token(tail...);
    }

    template<size_t N, class... Tail>
    bool In::is_at_token(const std::array<const char, N>& head, const Tail&... tail)
    {
        return (try_peek(N) && Aux::comp<N>(this->pos, head.data())) || this->is_at_token(tail...);
    }

    inline void In::move(size_t steps, const Error& error)
    {
        this->pos = this->peek(steps, error) + steps;
    }

    inline const uint8_t* In::peek(size_t steps, const Error& error)
    {
        if(this->pos + steps >= this->end) {
            this->fetch_data(steps, error);
        }
        return this->pos;
    }

    inline bool In::try_peek(size_t steps)
    {
        return this->pos + steps < this->end || this->try_fetch_data(steps);
    }

    inline MemBlock In::read_block(size_t steps, const Error& error)
    {
        this->anchor = this->pos;
        this->move(steps, error);
        this->anchor = nullptr;

        return { this->pos - steps, steps };
    }

    inline void In::skip_space(const Error& error)
    {
        this->move_while(error, ' ', '\n', '\t', '\r', '\b', '\f', '\b');
    }

    template<class... Tokens>
    size_t In::read_substitue(const Error& error, uint8_t delimiter,
                              std::vector<uint8_t>& buffer, const Tokens&... tokens)
    {
        const size_t max = Aux::max_len<Tokens...>();

        auto steps = 0U;
        auto bufsz = buffer.size();

        while(*this->peek(0, error) != delimiter) {

            if(srl_unlikely(steps + 1 > bufsz)) {
                bufsz += steps + 10;
                buffer.resize(bufsz);
            }

            auto left = this->try_peek(max) ? max : this->end - this->pos - 1;
            steps += this->substitute_token(buffer, steps, left, tokens...);
        }

        return steps;
    }

    template<class Sub, class Token, class... Tail>
    size_t In::substitute_token(std::vector<uint8_t>& buffer, size_t idx, size_t left,
                                const Sub& sub, const Token& token, const Tail&... tail)
    {
        const auto N = std::tuple_size<Token>::value;

        return left >= N && Aux::comp<N>(this->pos, token.data())
                ? this->replace<Sub, N>(sub, buffer, idx)
                : this->substitute_token(buffer, idx, left, tail...);
    }

    inline size_t In::substitute_token(std::vector<uint8_t>& buffer, size_t idx, size_t)
    {
        buffer[idx] = *this->pos++;
        return 1;
    }

    template<class Sub, size_t N>
    typename std::enable_if<!std::is_same<Sub, In::Substitute>::value, size_t>::type
    In::replace(const Sub& substitute, std::vector<uint8_t>& buffer, size_t idx)
    {
        buffer[idx] = substitute;
        this->pos += N;

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
        return this->pos;
    }

    inline bool In::is_streaming() const
    {
        return this->streaming;
    }
} }

#endif
