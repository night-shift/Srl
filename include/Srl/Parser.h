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

            SourceSeg(Value val_, Lib::MemBlock name_ = { }, bool buffered_ = false)
                : value(val_), name(name_), data_buffered(buffered_) { }

            SourceSeg(Type type)
                : SourceSeg(Value(type)) { }

            SourceSeg(Type type, Lib::MemBlock name_)
                : SourceSeg(Value(type), name_) { }

            Value         value;
            Lib::MemBlock name;
            bool          data_buffered;
        };

        virtual Format get_format() const = 0;

        virtual void parse_out(const Value& value, const Lib::MemBlock& name, Lib::Out& out) = 0;
        virtual SourceSeg parse_in(Lib::In& source) = 0;
    };
}

#endif
