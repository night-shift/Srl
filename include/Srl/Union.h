#ifndef SRL_UNION_H
#define SRL_UNION_H

#include "Exception.h"
#include "String.h"
#include "TpTools.h"

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

        /* saves boilerplate */
        void check_node(const String& name = "")
        {
            if(!nodeptr) {
                throw Exception("Field " + name.unwrap() + " is not a node.");
            }
        }

        void check_value(const String& name = "")

        {
            if(!valptr) {
                throw Exception("Field " + name.unwrap() + " is not a value.");
            }
        }

        Union operator [] (const String& name);

        template<class T>
        typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), T>::type
        unwrap();

        template<class T>
        typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), T>::type
        unwrap();


    private:
        Node*  nodeptr = nullptr;
        Value* valptr  = nullptr;
    };
}

#endif
