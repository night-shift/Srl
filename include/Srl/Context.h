#ifndef SRL_CONTEXT_H
#define SRL_CONTEXT_H

#include "Common.h"
#include "Enums.h"

namespace Srl {

    class Node;
    class String;

    class Context {

    public :
        Context(Node& node_, Mode mode_) : context_node(&node_), context_mode(mode_) { }

        template<class T>
        Context& operator () (T&& o);
        template<class T>
        Context& operator () (const String& name, T&& o);

        Node* node() const { return this->context_node; }
        Mode  mode() const { return this->context_mode; }

    private :
        Node* context_node;
        Mode  context_mode;

        size_t nodes_index  = 0;
        size_t values_index = 0;

        template<class T> void insert (const T& o, const String& name); 
        template<class T> void paste  (T& o, const String& name); 
    };
}

#endif
