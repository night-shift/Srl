#ifndef SRL_CONTEXT_H
#define SRL_CONTEXT_H

#include "Common.h"
#include "Enums.h"

namespace Srl {

    class Node;
    class String;

    template<Mode M> class Context;

    template<> class Context<Mode::Insert> {

    public :
        static const Mode mode = Mode::Insert;

        Context(Node& node_) : context_node(&node_) { }

        template<class T>
        Context& operator () (const T& o);
        template<class T>
        Context& operator () (const String& name, const T& o);

        Node* node() const { return this->context_node; }

    private :
        Node* context_node;
    };

    template<> class Context<Mode::Paste> {

    public :
        static const Mode mode = Mode::Paste;

        Context(const Node& node_) : context_node(&node_) { }

        template<class T>
        Context& operator () (T& o);
        template<class T>
        Context& operator () (const String& name, T& o);

        const Node* node() const { return this->context_node; }

    private :
        const  Node* context_node;
        size_t nodes_index  = 0;
        size_t values_index = 0;
    };
}

#endif
