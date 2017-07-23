#ifndef SRL_PARSERJSON_H
#define SRL_PARSERJSON_H

#include "Parser.h"

namespace Srl {

    class PJson : public Parser {

    public :
        PJson(bool compact_ = true)
            : compact(compact_) {  }

        Format get_format() const  override  { return Format::Text; }
        void   set_compact (bool val) { this->compact = val; }

        virtual void
        write(const Value& value, const Lib::MemBlock& name, Lib::Out& out) override;
        virtual std::pair<Lib::MemBlock, Value> read(Lib::In& source) override;
        virtual void clear() override;

    private :
        bool             compact;
        std::stack<Type> scope_stack;
        Type             scope_type   = Type::Null;
        bool             scope_closed = true;

        std::vector<uint8_t> name_buffer;
        std::vector<uint8_t> value_buffer;

        struct State {

            Lib::MemBlock name;
            Value         value = { Type::Null };

            bool complete      = false, name_processed = false,
                 reading_value = false;
        };

        struct LitTable {
            uint8_t table[256];
            LitTable();
        };

        LitTable lookup;

        void insert_spacing  (Type type, Lib::Out& out);

        void process_quote   (Lib::In& source, State& state);
        void process_bracket (char bracket, State& state);
        void process_char    (Lib::In& source, State& state, bool& out_move);

        void process_string  (const Lib::MemBlock& str, State& state);
        void process_literal (const Lib::MemBlock& str, State& state, Type hint);

        void throw_exception (State& state, const String& info);
    };
}

#endif
