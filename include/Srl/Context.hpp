#ifndef SRL_CONTEXT_HPP
#define SRL_CONTEXT_HPP

#include "Context.h"
#include "Node.h"
#include "String.h"

namespace Srl {

    template<class T>
    StoreContext& StoreContext::operator () (const T& o)
    {
        return this->operator()(String(), o);
    }

    template<class T>
    RestoreContext& RestoreContext::operator () (T&& o)
    {
        return this->operator()(String(), o);
    }

    template<class T>
    StoreContext& StoreContext::operator () (const String& name, const T& o, uint64_t flags)
    {
        this->context.insert(o, name, flags);
        return *this;
    }

    template<class T>
    RestoreContext& RestoreContext::operator () (const String& name, T&& o, uint64_t flags)
    {
        this->context.paste(o, name, flags);
        return *this;
    }

    template<class T>
    Context& Context::operator () (T&& o)
    {
        return this->operator()(String(), o);
    }

    template<class T>
    Context& Context::operator () (const String& name, T&& o, uint64_t flags)
    {
        if(this->context_mode == Mode::Insert) {
            this->insert(o, name, flags);

        } else {
            this->paste(o, name, flags);
        }

        return *this;
    }

    template<class T>
    void Context::insert(const T& o, const String& name, uint64_t)
    {
        this->context_node->insert(name, o);
    }

    template<class T>
    void Context::paste(T& o, const String& name, uint64_t flags)
    {
        auto type   = Lib::Switch<T>::type;
        auto& index = TpTools::is_scope(type) ? this->nodes_index : this->values_index;

        if((flags & CtxFlags::Optional)) {

            bool has = TpTools::is_scope(type)
                ? context_node->has_node(name)
                : context_node->has_value(name);

            if(!has) {
                return;
            }
        }

        if(name.size() > 0) {
            this->context_node->paste_field(name, o);

        } else {
            this->context_node->paste_field(index, o);
        }

        index += 1;
    }
}

#endif
