#ifndef SRL_TREE_HPP
#define SRL_TREE_HPP

#include "Tree.h"
#include "Context.h"
#include "String.h"
#include "Tools.h"
#include "Node.h"
#include "Out.h"

namespace Srl {

    template<class TParser>
    void Tree::to_source(Lib::Out::Source source, TParser&& parser)
    {
        this->root_node->to_source(source, parser);
    }

    template<class TParser>
    std::vector<uint8_t> Tree::to_source(TParser&& parser)
    {
        return this->root_node->to_source(parser);
    }

    template<class TParser>
    void Tree::load_source(Lib::In::Source source, TParser&& parser)
    {
        this->read_source(parser, source);
    }

    template<class TParser>
    void Tree::load_source(const char* data, size_t data_len,  TParser&& parser)
    {
        return load_source(Lib::In::Source((const uint8_t*)data, data_len), parser);
    }

    template<class TParser>
    void Tree::load_source(const uint8_t* data, size_t data_len,  TParser&& parser)
    {
        return load_source(Lib::In::Source(data, data_len), parser);
    }

    namespace Lib { namespace Aux {
        template<class T, class = void> struct InsertSwitch {
            static void Insert(const T& o, Tree& tree) { tree.root().insert(o); };
        };
        template<class T> struct
        InsertSwitch<T, typename std::enable_if<TpTools::is_scope(Switch<T>::type)>::type> {
            static void Insert(const T& o, Tree& tree) {
                Switch<T>::Insert(tree.root(), o);
            }
        };
    } }

    template<class T>
    void Tree::load_object(const T& type)
    {
        this->clear();
        Lib::Aux::InsertSwitch<T>::Insert(type, *this);
    }
}

#endif
