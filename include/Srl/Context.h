#ifndef SRL_CONTEXT_H
#define SRL_CONTEXT_H

#include "Common.h"
#include "Enums.h"

namespace Srl {

    class Node;
    class String;
    class StoreContext;
    class RestoreContext;

    namespace CtxFlags {
        const size_t None     = 0;
        const size_t Optional = 1;
    }

    class Context {

    friend class StoreContext;
    friend class RestoreContext;

    public :
        Context(Node& node_, Mode mode_) : context_node(&node_), context_mode(mode_) { }

        template<class T>
        Context& operator () (T&& o);
        template<class T>
        Context& operator () (const String& name, T&& o, size_t flags = CtxFlags::None);

        Node& node() const { return *this->context_node; }
        Mode  mode() const { return this->context_mode; }

    private :
        Node* context_node;
        Mode  context_mode;

        size_t nodes_index  = 0;
        size_t values_index = 0;

        template<class T> void insert (const T& o, const String& name, size_t flags);
        template<class T> void paste  (T& o, const String& name, size_t flags);
    };


    class StoreContext {
    public:
        StoreContext(Node& node) : context(Context(node, Mode::Insert)) { }

        template<class T>
        StoreContext& operator () (const T& o);
        template<class T>
        StoreContext& operator () (const String& name, const T& o, size_t flags = CtxFlags::None);

    private:
        Context context;
    };

    class RestoreContext {
    public:
        RestoreContext(Node& node) : context(Context(node, Mode::Paste)) { }

        template<class T>
        RestoreContext& operator () (T&& o);
        template<class T>
        RestoreContext& operator () (const String& name, T&& o, size_t flags = CtxFlags::None);

    private:
        Context context;
    };
}

#endif
