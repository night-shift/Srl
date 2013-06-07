#ifndef SRL_PARSERBSON_H
#define SRL_PARSERBSON_H

#include "Parser.h"

namespace Srl {

    class PBson : public Parser {

    public :
        PBson() { }

        Format get_format() const { return Format::Binary; }

        virtual void
        parse_out(const Value& value, const Lib::MemBlock& name, Lib::Out& out) override;
        virtual SourceSeg parse_in(Lib::In& source) override;

    private :
        struct Scope {
            Scope(Type type_, const Lib::Out::Ticket& ticket_)
                : type(type_), ticket(ticket_) {  };

            Type     type;
            size_t   document_size = 0,
                     elements      = 0;

            Lib::Out::Ticket ticket;
        };

        std::stack<Scope>     scope_stack;
        std::stack<Type>      type_stack;
        std::vector<uint8_t>  buffer;

        void insert         (const uint8_t* data, size_t size, Lib::Out& out);
        void insert_prefix  (uint8_t prefix, const Lib::MemBlock& name, Lib::Out& out);
        void insert_scope   (Type type, Lib::Out& out);
        void insert_literal (Lib::MemBlock& block, Type type, Lib::Out& out);
        void insert_binary  (Lib::MemBlock& block, Lib::Out& out);
        void insert_string  (Lib::MemBlock& block, Lib::Out& out);
    };
}
#endif
