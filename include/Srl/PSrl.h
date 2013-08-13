#ifndef SRL_PARSERSRL_H
#define SRL_PARSERSRL_H

#include "Common.h"
#include "Parser.h"
#include "Blocks.h"
#include "Hash.h"

namespace Srl {

    namespace Lib { 
        template<> struct HashSrl<MemBlock> {
            inline size_t operator() (const MemBlock& s) const
            {
                return Aux::hash_fnc(s.ptr, s.size);
            }
        };
    }

    class PSrl : public Parser {

    public :
        PSrl() { }

        Format get_format() const { return Format::Binary; }

        virtual void
        write(const Value& value, const Lib::MemBlock& name, Lib::Out& out) override;
        virtual std::pair<Lib::MemBlock, Value> read(Lib::In& source) override;

    private :
        Type   scope     = Type::Null;
        size_t n_strings = 0;

        std::vector<Lib::MemBlock>         indexed_strings;
        Lib::Heap                          string_buffer;
        std::stack<Type>                   scope_stack;
        Lib::HTable<Lib::MemBlock, size_t> hashed_strings;

        void push_scope (Type scope_type);
        void pop_scope  ();

        std::pair<Lib::MemBlock, Value> read_num (uint8_t flag, const Lib::MemBlock& name, Lib::In& source);

        void write_head (uint8_t flag, const Lib::MemBlock& str, Lib::Out& out);
        std::pair<uint8_t, Lib::MemBlock> read_head (Lib::In& source);
    };
}

#endif






