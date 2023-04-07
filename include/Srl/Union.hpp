#ifndef SRL_UNION_HPP
#define SRL_UNION_HPP

#include "Union.h"
#include "Node.h"

namespace Srl {

    inline Union Union::operator[] (const String& name)
    {
        check_node(name);
        return this->nodeptr->field(name);
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
}

#endif
