#ifndef SRL_PARSERSRL_H
#define SRL_PARSERSRL_H

#include "Common.h"
#include "Parser.h"
#include "Blocks.h"
#include "TpTools.hpp"

namespace Srl {

    class PSrl : public Parser {

    public :
        PSrl() { }

        Format get_format() const { return Format::Binary; }

        virtual void
        parse_out(const Value& value, const Lib::MemBlock& name, Lib::Out& out) override;
        virtual SourceSeg parse_in(Lib::In& source) override;

    private :
        enum class StringPrefix : uint8_t {
            Empty   = 0,
            New     = 1,
            Indexed = 2
        };
        inline bool valid_string_prefix (uint8_t token) { return token < 3; }

        Type   scope            = Type::Null;
        size_t n_hashed_strings = 0;

        uint8_t integer_buffer[TpTools::Literal_Max_Size];

        std::map<size_t, size_t>   hashed_strings;
        std::vector<Lib::MemBlock> indexed_strings;
        Lib::Heap                  string_buffer;
        std::stack<Type>           scope_stack;

        void push_scope (Type scope_type);
        void pop_scope  ();

        SourceSeg read_literal (const Lib::MemBlock& name, Lib::In& source, Type type);
        void write_string (const Lib::MemBlock& str, Lib::Out& out);
        Lib::MemBlock read_string (Lib::In& source);
    };
}

#endif






