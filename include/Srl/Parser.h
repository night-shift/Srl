#ifndef SRL_PARSER_H
#define SRL_PARSER_H

#include "Common.h"
#include "Enums.h"
#include "Out.h"
#include "Value.h"

namespace Srl {

    namespace Lib {
        class In;
    }

    struct Parser {

        struct SourceSeg {

            SourceSeg(Value val_, Lib::MemBlock name_ = { })
                : value(val_), name(name_) { }

            SourceSeg(Type type)
                : SourceSeg(Value(type)) { }

            Value         value;
            Lib::MemBlock name;
        };

        virtual Format get_format() const = 0;

        virtual void parse_out(const Value& value, const Lib::MemBlock& name, Lib::Out& out) = 0;
        virtual SourceSeg parse_in(Lib::In& source) = 0;
    };
}

#endif
