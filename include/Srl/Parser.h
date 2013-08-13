#ifndef SRL_PARSER_H
#define SRL_PARSER_H

#include "Common.h"
#include "Enums.h"
#include "Value.h"

namespace Srl {

    namespace Lib {
        class In;
        class Out;
    }

    struct Parser {

        struct Guard {

            Guard(Parser& parser_) : parser(&parser_) { }

            ~Guard()
            { 
                parser->clear();
            }

            Parser* parser;
        };

        virtual Format get_format() const = 0;

        virtual void write(const Value& value, const Lib::MemBlock& name, Lib::Out& out) = 0;
        virtual std::pair<Lib::MemBlock, Value> read(Lib::In& source) = 0;
        virtual void clear() = 0;
    };
}

#endif
