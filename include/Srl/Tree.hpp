#ifndef SRL_TREE_HPP
#define SRL_TREE_HPP

#include "Tree.h"
#include "Context.h"
#include "String.h"
#include "Tools.h"
#include "Out.h"

namespace Srl {

    template<class TParser>
    std::vector<uint8_t> Tree::to_source(const TParser& parser)
    {
        TParser copy = parser;
        Lib::Out out;
        this->parse_out(copy, out);

        return out.extract();
    }

    template<class TParser>
    void Tree::to_source(std::ostream& out_stream, const TParser& parser)
    {
        TParser copy = parser;
        Lib::Out out(out_stream);
        this->parse_out(copy, out);

        out.flush();
    }

    template<class TParser>
    Tree Tree::From_Source (std::istream& stream, const TParser& parser)
    {
        Lib::In source(stream);
        return Tree::From_Source<TParser>(source, parser);
    }

    template<class TParser>
    Tree Tree::From_Source(const std::vector<uint8_t>& source, const TParser& parser)
    {
        Lib::In src(&source[0], source.size());
        return Tree::From_Source(src, parser);
    }

    template<class TParser>
    Tree Tree::From_Source(const uint8_t* source, size_t source_size, const TParser& parser)
    {
        Lib::In src(source, source_size);
        return Tree::From_Source(src, parser);
    }

    template<class TParser>
    Tree Tree::From_Source(Lib::In& source, const TParser& parser)
    {
        TParser copy = parser;
        Tree tree;
        tree.parse_in(copy, source);

        return std::move(tree);
    }

    namespace Lib { namespace Aux {
        template<class T, class = void> struct InsertSwitch {
            static void Insert(T& o, Tree& tree) { tree.root()->insert(o); };
        };
        template<class T> struct
        InsertSwitch<T, typename std::enable_if<has_resolve_method<T>::value>::type> {
            static void Insert(T& o, Tree& tree) {
                Context<Mode::Insert> ctx(*tree.root());
                o.srl_resolve(ctx);
            }
        };
    } }

    template<class T>
    Tree Tree::From_Type(T& type, const std::string name)
    {
        Tree tree(name);
        Lib::Aux::InsertSwitch<T>::Insert(type, tree);

        return std::move(tree);
    }
}

#endif
