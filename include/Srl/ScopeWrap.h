#ifndef SRL_SCOPEWRAP_H
#define SRL_SCOPEWRAP_H

#include "Type.h"
#include "TpTools.h"
#include <functional>

namespace Srl {

    class Node;

    class ScopeWrap {

    public:
        ScopeWrap(const std::function<void(Node&)>& insertfnc_, Type tp = Type::Object)
            : scope_type(tp), insertfnc(insertfnc_)
        {
        }

        void insert(Node& node) const { insertfnc(node); }

        Type type() const { return scope_type; }

    private:
        const Type scope_type;
        const std::function<void(Node&)>& insertfnc;

    };
}

#endif
