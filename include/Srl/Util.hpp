#ifndef SRL_UTIL_HPP
#define SRL_UTIL_HPP

#include "Util.h"
#include "Parser.h"
#include "Context.h"
#include "Tree.h"

namespace Srl {

    namespace Lib { namespace Aux {

        template<class T, class = void> struct TypeSwitch {
            static const Type type = Type::Object;

            static std::function<void()> Insert(const T& o, Tree& tree) {
                return [&o, &tree] { tree.root().insert("value", o); };
            }
            static std::function<void()> Paste(Tree& tree, T& o) {
                return [&o, &tree] { tree.root().paste_field(0, o); };
            }
        };

        template<class T>
        struct TypeSwitch<T, typename std::enable_if<TpTools::is_scope(Switch<T>::type)>::type> {
            static const Type type = Switch<T>::type;

            static std::function<void()> Insert(const T& o, Tree& tree) {
                return [&o, &tree] { Switch<T>::Insert(tree.root(), o); };
            }
            static std::function<void()> Paste(Tree& tree, T& o) {
                return [&o, &tree] { tree.root().paste(o); };
            }
        };

    } }

    template<class TParser, class Object>
    std::vector<uint8_t> Store(const Object& object, TParser&& parser)
    {
        Lib::Out out;
        Store<Object>(object, out, parser);
        return out.extract();
    }

    template<class TParser, class Object>
    void Store(std::ostream& out_stream, const Object& object, TParser&& parser)
    {
        Lib::Out out(out_stream);
        Store<Object>(object, out, parser);
        out.flush();
    }

    template<class Object>
    void Store(const Object& object, Lib::Out& out, Parser& parser)
    {
        Tree tree;
        tree.write(Lib::Aux::TypeSwitch<Object>::type, parser, out,
                   Lib::Aux::TypeSwitch<Object>::Insert(object, tree));
    }

    template<class Object, class TParser>
    Object Restore(std::istream& stream, TParser&& parser)
    {
        auto object = Ctor<Object>::Create();
        Restore<Object, TParser>(object, stream, parser);

        return std::move(object);
    }

    template<class Object, class TParser>
    Object Restore(const std::vector<uint8_t>& vec, TParser&& parser)
    {
        return Restore<Object>(vec.data(), vec.size(), parser);
    }

    template<class Object, class TParser>
    Object Restore(const uint8_t* data, size_t size, TParser&& parser)
    {
        auto object = Ctor<Object>::Create();
        Lib::In in(data, size);
        Restore<Object>(object, in, parser);

        return std::move(object);
    }

    template<class TParser, class Object>
    void Restore(Object& object, const std::vector<uint8_t>& vec, TParser&& parser)
    {
        Restore(object, vec.data(), vec.size(), parser);
    }

    template<class TParser, class Object>
    void Restore(Object& object, const uint8_t* data, size_t size, TParser&& parser)
    {
        Lib::In in(data, size);
        Restore(object, in, parser);
    }

    template<class TParser, class Object>
    void Restore(Object& object, std::istream& stream, TParser&& parser)
    {
        Lib::In in(stream);
        Restore(object, in, parser);
    }

    template<class Object>
    void Restore(Object& object, Lib::In& in, Parser& parser)
    {
        Tree tree;
        tree.read_source(parser, in, Lib::Aux::TypeSwitch<Object>::Paste(tree, object));
    }
}

#endif
