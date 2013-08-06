#ifndef SRL_CONTEXT_HPP
#define SRL_CONTEXT_HPP

#include "Context.h"
#include "Node.h"
#include "String.h"

namespace Srl {

    template<class T>
    Context& Context::operator () (T&& o)
    {
        return this->operator()(String(), o);
    }

    template<class T>
    Context& Context::operator () (const String& name, T&& o)
    {
        if(this->context_mode == Mode::Insert) {
            this->insert(o, name);

        } else {
            this->paste(o, name);
        }

        return *this;
    }

    template<class T>
    void Context::insert(const T& o, const String& name)
    {
        this->context_node->insert(name, o);
    }

    template<class T>
    void Context::paste(T& o, const String& name)
    {
        auto type   = Lib::Switch<T>::type;
        auto& index = TpTools::is_scope(type) ? this->nodes_index : this->values_index;

        if(name.size() > 0) {
            this->context_node->paste_field(name, o);

        } else {
            this->context_node->paste_field(index, o);
        }

        index += 1;
    }
}

#endif
