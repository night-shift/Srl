#ifndef SRL_TREE_HPP
#define SRL_TREE_HPP

#include "Tree.h"
#include "Context.h"
#include "String.h"
#include "Tools.h"
#include "Out.h"

namespace Srl {

    template<class TParser>
    std::vector<uint8_t> Tree::to_source(TParser&& parser)
    {
        return this->root_node->to_source(parser);
    }

    template<class TParser>
    void Tree::to_source(std::ostream& out_stream, TParser&& parser)
    {
        this->root_node->to_source(out_stream, parser);
    }

    template<class TParser>
    void Tree::load_source(std::istream& stream, TParser&& parser)
    {
        Lib::In source(stream);
        this->load_source<TParser>(source, parser);
    }

    template<class TParser>
    void Tree::load_source(const std::vector<uint8_t>& source, TParser&& parser)
    {
        Lib::In src(&source[0], source.size());
        this->load_source(src, parser);
    }

    template<class TParser>
    void Tree::load_source(const uint8_t* source, size_t source_size, TParser&& parser)
    {
        Lib::In src(source, source_size);
        this->load_source(src, parser);
    }

    template<class TParser>
    void Tree::load_source (const char* source, size_t source_size, TParser&& parser)
    {
        this->load_source((const uint8_t*)source, source_size, parser);
    }

    template<class TParser>
    void Tree::load_source(Lib::In& source, TParser& parser)
    {
        this->clear();
        this->read_source(parser, source);
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
