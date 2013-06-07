#ifndef SRL_PARSERJSON_H
#define SRL_PARSERJSON_H

#include "Common.h"
#include "Enums.h"
#include "Parser.h"
#include "Blocks.h"
#include "Value.h"
#include "String.h"
#include "TpTools.hpp"

namespace Srl {

    class PJson : public Parser {

    public :
        PJson(bool insert_whitespace_ = true)
            : insert_whitespace(insert_whitespace_) {  }

        Format get_format() const { return Format::Text; }

        virtual void
        parse_out(const Value& value, const Lib::MemBlock& name, Lib::Out& out) override;
        virtual SourceSeg parse_in(Lib::In& source) override;

    private :
        bool             insert_whitespace;
        uint8_t          conversion_buffer[TpTools::Literal_Max_Size];
        std::stack<Type> scope_stack;
        Type             scope_type   = Type::Null;
        bool             scope_closed = true;

        std::vector<uint8_t> name_buffer;
        std::vector<uint8_t> value_buffer;

        struct State {

            Lib::MemBlock name;
            Value         parsed_value = { Type::Null };

            bool complete      = false, name_processed = false,
                 reading_value = false;
        };

        struct LiteralTable {
            static const uint8_t Char_Invalid = 0;
            static const uint8_t Char_Valid   = 1;

            uint8_t table[256];

            LiteralTable();
        };

        LiteralTable lookup;

        void insert_spacing  (Type type, Lib::Out& out);

        void process_quote   (Lib::In& source, State& state);
        void process_bracket (char bracket, State& state);
        void process_char    (Lib::In& source, State& state, bool& out_move);

        void process_string  (const Lib::MemBlock& data, State& state);
        void process_literal (const Lib::MemBlock& data, State& state);

        void throw_exception (State& state, const String& info);
    };
}

#endif
