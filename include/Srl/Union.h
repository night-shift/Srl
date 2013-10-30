#ifndef SRL_UNION_H
#define SRL_UNION_H

#include "Exception.h"

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
        void check_node()
        {
            if(!nodeptr) {
                throw Exception("Union is not a node.");
            }
        }

        void check_value()
        {
            if(!valptr) {
                throw Exception("Union is not a value.");
            }
        }

    private:
        Node*  nodeptr = nullptr;
        Value* valptr  = nullptr;
    };
}

#endif
