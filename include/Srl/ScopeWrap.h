#ifndef SRL_SCOPEWRAP_H
#define SRL_SCOPEWRAP_H

#include "Type.h"
#include "TpTools.h"
#include <functional>

namespace Srl {

    class Node;

    class ScopeWrap {

    public:
        ScopeWrap(const std::function<void(Node&)>& fn_insert_ = [](auto&) { }, Type tp = Type::Object)
            : scope_type(tp), fn_insert(fn_insert_)
        {
        }

        void insert(Node& node) const { fn_insert(node); }

        Type type() const { return scope_type; }

    private:
        const Type scope_type;
        const std::function<void(Node&)>& fn_insert;

    };
}

#endif
