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

        template<class TParser, typename Object>
        void Store(const Object& object, Lib::Out& out, const TParser& parser, const std::string& name)
        {
            TParser copy = parser;
            Tree tree(name);
            tree.parse_out(copy, out, StoreSwitch<Object>::Insert(object, tree));
        }
    } }

    template<class TParser, class Object>
    std::vector<uint8_t> Store(const Object& object, const TParser& parser, const std::string& name)
    {
        Lib::Out out;
        Lib::Aux::Store(object, out, parser, name);
        return out.extract();
    }

    template<class TParser, typename Object>
    void Store(std::ostream& out_stream, const Object& object, const TParser& parser, const std::string& name)
    {
        Lib::Out out(out_stream);
        Lib::Aux::Store(object, out, parser, name);
        out.flush();
    }

    template<class Object, class TParser>
    Object Restore(std::istream& stream, const TParser& parser)
    {
        auto object = Ctor<Object>::Create();
        Restore(object, stream, parser);

        return std::move(object);
    }

    template<class Object, class TParser>
    Object Restore(const std::vector<uint8_t>& source, const TParser& parser)
    {
        return Restore<Object, TParser>(source.data(), source.size(), parser);
    }

    template<class Object, class TParser>
    Object Restore(const uint8_t* source, size_t size, const TParser& parser)
    {
        auto object = Ctor<Object>::Create();
        Lib::In in(source, size);
        Restore(object, in, parser);

        return std::move(object);
    }

    template<class TParser, class Object>
    void Restore(Object& object, const std::vector<uint8_t>& source, const TParser& parser)
    {
        Restore(object, source.data(), source.size(), parser);
    }

    template<class TParser, class Object>
    void Restore(Object& object, const uint8_t* source, size_t size, const TParser& parser)
    {
        Lib::In in(source, size);
        Restore(object, source, parser);
    }

    template<class TParser, class Object>
    void Restore(Object& object, std::istream& stream, const TParser& parser)
    {
        Lib::In source(stream);
        Restore(object, source, parser);
    }

    template<class Object, class TParser>
    void Restore(Object& object, Lib::In& source, const TParser& parser)
    {
        TParser copy = parser;
        Tree tree;
        bool just_parse = true;
        tree.parse_in(copy, source, just_parse);

        Lib::Aux::StoreSwitch<Object>::Paste(tree, object);
    }
}

#endif
