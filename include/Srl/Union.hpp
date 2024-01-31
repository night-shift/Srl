#ifndef SRL_UNION_HPP
#define SRL_UNION_HPP

#include "Union.h"
#include "Node.h"
#include <optional>

namespace Srl {

    inline Union Union::get(const String& name)
    {
        check_node(name);
        return this->nodeptr->get(name);
    }

    inline std::optional<Union> Union::try_get(const String& name)
    {
        if(!this->is_node()) {
            return { };
        }
        return this->nodeptr->try_get(name);
    }

    template<class T>
    Union::operator T()
    {
        return this->unwrap<T>();
    }

    template<class T>
    typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), T>::type
    Union::unwrap()
    {
        check_value();
        return this->valptr->unwrap<T>();
    }

    template<class T>
    typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), T>::type
    Union::unwrap()
    {
        check_node();
        return this->nodeptr->unwrap<T>();
    }

    template<class T>
    typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), void>::type
    Union::copy_to(T& v)
    {
        check_value();
        this->value()->paste(v);
    }

    template<class T>
    typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), void>::type
    Union::copy_to(T& n)
    {
        check_node();
        this->node()->paste(n);
    }

    template<class T>
    std::optional<typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), T>::type>
    Union::unwrap_if()
    {
        if(this->is_value()) {

            try {
                T res = this->valptr->unwrap<T>();
                return { std::move(res) };

            } catch(...) { }
        }

        return { };
    }

    template<class T>
    std::optional<typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), T>::type>
    Union::unwrap_if()
    {
        if(this->is_node()) {

            try {
                T res = this->nodeptr->unwrap<T>();
                return { std::move(res) };

            } catch(...) { }
        }

        return { };
    }

}

#endif
