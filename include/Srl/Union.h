#ifndef SRL_UNION_H
#define SRL_UNION_H

#include "Srl/Environment.h"
#include "String.h"
#include "TpTools.h"
#include <exception>
#include <optional>

namespace Srl {

    class Node;
    class Value;

    class Union {

    public:
        Union() { }

        Union(Node& node_)   : nodeptr(&node_) { }
        Union(Value& value_) : valptr(&value_) { }

        Node*  node()  { return nodeptr; }
        Value* value() { return valptr; }

        bool is_node() { return nodeptr != nullptr; }
        bool is_value() { return valptr != nullptr; }

        template<class T>
        operator T();

        Union get (const String& name);
        std::optional<Union> try_get (const String& name);

        template<class T>
        typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), T>::type
        unwrap();

        template<class T>
        typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), T>::type
        unwrap();

        template<class T>
        typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), void>::type
        copy_to(T&);

        template<class T>
        typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), void>::type
        copy_to(T&);

        template<class T>
        std::optional<typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), T>::type>
        unwrap_if();

        template<class T>
        std::optional<typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), T>::type>
        unwrap_if();

    private:
        Node*  nodeptr = nullptr;
        Value* valptr  = nullptr;

        void check_not_empty();
        void check_node();
        void check_value();

        template<class T>
        Exception make_conversion_error(std::exception& prev_ex);


    };
}

#endif
