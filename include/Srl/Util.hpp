#ifndef SRL_UTIL_HPP
#define SRL_UTIL_HPP

#include "Util.h"
#include "Parser.h"
#include "Context.h"
#include "Tree.h"

namespace Srl {

    namespace Lib { namespace Aux {

        template<class T, class = void> struct StoreSwitch {
            static std::function<void()> Insert(const T& o, Tree& tree) {
                return [&o, &tree] { tree.root()->insert(o); };
            }
            static void Paste(Tree& tree, T& target) {
                tree.root()->paste_field(0, target);
            }
        };
        template<class T>
        struct StoreSwitch<T, typename std::enable_if<has_resolve_method<T>::value>::type> {
            static std::function<void()> Insert(const T& o, Tree& tree) {
                return [&o, &tree] {
                    Switch<T>::Insert(*tree.root(), o);
                };
            }
            static void Paste(Tree& tree, T& target) {
                tree.root()->paste(target);
            }
        };

        template<class TParser, typename Data>
        void Store(const Data& data, Lib::Out& out, const TParser& parser, const std::string& name)
        {
            TParser copy = parser;
            Tree tree(name);
            tree.parse_out(copy, out, StoreSwitch<Data>::Insert(data, tree));
        }
    } }

    template<class TParser, class Data>
    std::vector<uint8_t> Store(const Data& data, const TParser& parser, const std::string& name)
    {
        Lib::Out out;
        Lib::Aux::Store(data, out, parser, name);
        return out.extract();
    }

    template<class TParser, typename Data>
    void Store(std::ostream& out_stream, const Data& data, const TParser& parser, const std::string& name)
    {
        Lib::Out out(out_stream);
        Lib::Aux::Store(data, out, parser, name);
        out.flush();
    }

    template<class Data, class TParser>
    Data Restore(const std::vector<uint8_t>& source, const TParser& parser)
    {
        return Restore<Data, TParser>(&source[0], source.size(), parser);
    }

    template<class Data, class TParser>
    Data Restore(const uint8_t* source, size_t size, const TParser& parser)
    {
        auto data = Ctor<Data>::Create();
        Restore<Data, TParser>(data, source, size, parser);

        return std::move(data);
    }

    template<class TParser, class Data>
    void Restore(Data& data, const std::vector<uint8_t>& source, const TParser& parser)
    {
        Restore(data, &source[0], source.size(), parser);
    }

    template<class Data, class TParser>
    void Restore(Data& data, const uint8_t* source, size_t size, const TParser& parser)
    {
        TParser copy = parser;
        Tree tree;
        bool just_parse = true;
        Lib::In src(source, size);
        tree.parse_in(copy, src, just_parse);

        Lib::Aux::StoreSwitch<Data>::Paste(tree, data);
    }
}

#endif
