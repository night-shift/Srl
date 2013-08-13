#ifndef SRL_PMSGPACK
#define SRL_PMSGPACK

#include "Parser.h"
#include "Out.h"

namespace Srl {

    class PMsgPack : public Parser {

    public :
        PMsgPack() { }

        Format get_format() const { return Format::Binary; }

        virtual void
        write (const Value& value, const Lib::MemBlock& name, Lib::Out& out) override;
        virtual std::pair<Lib::MemBlock, Value> read (Lib::In& source) override;
        virtual void clear() override;

    private:
        struct Scope {
            Scope(Type type_, size_t elements_, const Lib::Out::Ticket& ticket_ = { })
                : type(type_), elements(elements_), ticket(ticket_) { }
            Type             type;
            uint32_t         elements;
            Lib::Out::Ticket ticket;
        };

        std::vector<uint8_t>  buffer;
        std::stack<Scope>     scope_stack;

        Scope* scope = nullptr;

        void write_scope (Type type, Lib::Out& out);
    };
}

#endif
