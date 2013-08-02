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
                return [&o, &tree] { tree.root().insert(o); };
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
    std::vector<uint8_t> Store(const Object& object, const TParser& parser, const std::string& name)
    {
        Lib::Out out;
        Store<TParser, Object>(object, out, parser, name);
        return out.extract();
    }

    template<class TParser, class Object>
    void Store(std::ostream& out_stream, const Object& object, const TParser& parser, const std::string& name)
    {
        Lib::Out out(out_stream);
        Store<TParser, Object>(object, out, parser, name);
        out.flush();
    }

    template<class TParser, class Object>
    void Store(const Object& object, Lib::Out& out, const TParser& parser, const std::string& name)
    {
        TParser copy = parser;
        Tree tree(name);
        tree.write(Lib::Aux::TypeSwitch<Object>::type, copy, out,
                   Lib::Aux::TypeSwitch<Object>::Insert(object, tree));
    }

    template<class Object, class TParser>
    Object Restore(std::istream& stream, const TParser& parser)
    {
        auto object = Ctor<Object>::Create();
        Restore<Object, TParser>(object, stream, parser);

        return std::move(object);
    }

    template<class Object, class TParser>
    Object Restore(const std::vector<uint8_t>& vec, const TParser& parser)
    {
        return Restore<Object, TParser>(vec.data(), vec.size(), parser);
    }

    template<class Object, class TParser>
    Object Restore(const uint8_t* data, size_t size, const TParser& parser)
    {
        auto object = Ctor<Object>::Create();
        Lib::In in(data, size);
        Restore<Object, TParser>(object, in, parser);

        return std::move(object);
    }

    template<class TParser, class Object>
    void Restore(Object& object, const std::vector<uint8_t>& vec, const TParser& parser)
    {
        Restore(object, vec.data(), vec.size(), parser);
    }

    template<class TParser, class Object>
    void Restore(Object& object, const uint8_t* data, size_t size, const TParser& parser)
    {
        Lib::In in(data, size);
        Restore(object, in, parser);
    }

    template<class TParser, class Object>
    void Restore(Object& object, std::istream& stream, const TParser& parser)
    {
        Lib::In in(stream);
        Restore(object, in, parser);
    }

    template<class Object, class TParser>
    void Restore(Object& object, Lib::In& in, const TParser& parser)
    {
        TParser copy = parser;
        Tree tree;
        tree.read_source(copy, in, Lib::Aux::TypeSwitch<Object>::Paste(tree, object));
    }
}

#endif
