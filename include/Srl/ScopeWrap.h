#ifndef SRL_SCOPEWRAP_H
#define SRL_SCOPEWRAP_H

#include "Type.h"
#include "TpTools.h"
#include <functional>

namespace Srl {

    class Node;

    class ScopeWrap {

    public:
        template<Type TP = Type::Object>
        ScopeWrap(const std::function<void(Node&)>& insertfnc_)
            : scope_type(TP), insertfnc(insertfnc_)
        {
            static_assert(TpTools::is_scope(TP), "Scope can only be of type Object or Array.");
        }

        void insert(Node& node) const { insertfnc(node); }

        Type type() const { return scope_type; }

    private:
        const Type scope_type;
        const std::function<void(Node&)>& insertfnc;

    };
}

#endif
