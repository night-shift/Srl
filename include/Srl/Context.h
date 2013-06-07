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

        Context(Node& node_) : node(&node_) { }

        template<class T>
        Context& operator () (const T& o);
        template<class T>
        Context& operator () (const T& o, const String& name);

    private :
        Node* node;

    };

    template<> class Context<Mode::Paste> {

    public :
        static const Mode mode = Mode::Paste;

        Context(const Node& node_) : node(&node_) { }

        template<class T>
        Context& operator () (T& o);
        template<class T>
        Context& operator () (T& o, const String& name);

    private :
        const  Node* node;
        size_t node_index  = 0;
        size_t value_index = 0;
    };
}

#endif
