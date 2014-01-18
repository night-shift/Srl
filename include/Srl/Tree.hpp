#ifndef SRL_TREE_HPP
#define SRL_TREE_HPP

#include "Tree.h"
#include "Context.h"
#include "String.h"
#include "Tools.h"
#include "Node.h"
#include "Out.h"

namespace Srl {

    namespace Lib { namespace Aux {

        template<class T, class = void> struct TypeSwitch {
            static const Type type = Type::Object;

            static void Insert(const T& o, Tree& tree) { tree.root().insert(o); };

            static std::function<void()> Store(const T& o, Tree& tree) {
                return [&o, &tree] { tree.root().insert("value", o); };
            }
            static std::function<void()> Restore(Tree& tree, T& o) {
                return [&o, &tree] { tree.root().paste_field(0, o); };
            }
        };

        template<class T> struct
        TypeSwitch<T, typename std::enable_if<TpTools::is_scope(Switch<T>::type)>::type> {
            static const Type type = Switch<T>::type;

            static void Insert(const T& o, Tree& tree) {
                Switch<T>::Insert(tree.root(), o);
            }

            static std::function<void()> Store(const T& o, Tree& tree) {
                return [&o, &tree] { Switch<T>::Insert(tree.root(), o); };
            }
            static std::function<void()> Restore(Tree& tree, T& o) {
                return [&o, &tree] { tree.root().paste(o); };
            }
        };

    } }

    template<class TParser, class... Items>
    void Tree::pack(Lib::Out::Source out, TParser&& parser, const Items&... items)
    {
        if(this->env) {
            this->clear();

        } else {
            this->create_env();
        }

        this->env->set_output(parser, out);
        this->env->parsing = true;

        this->env->write(Value(Type::Object), "");

        this->root_node->insert(items...);

        this->env->write(Value(Type::Scope_End), "");

        this->env->parsing = false;
        this->env->out.flush();
    }

    template<class TParser, class... Items>
    void Tree::unpack(Lib::In::Source in, TParser&& parser, Items&... items)
    {
        this->prologue_in(parser, in);

        this->root_node->parsed = false;

        this->root_node->paste(items...);
    }

    template<class TParser>
    void Tree::to_source(Lib::Out::Source source, TParser&& parser)
    {
        if(!this->env) {
            this->create_env();
        }
        this->root_node->to_source(source, parser);
    }

    template<class TParser>
    std::vector<uint8_t> Tree::to_source(TParser&& parser)
    {
        if(!this->env) {
            this->create_env();
        }
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

    template<class TParser, class T>
    void Tree::store(const T& object, Lib::Out::Source out, TParser&& parser)
    {
        this->to_source(
            Lib::Aux::TypeSwitch<T>::type, parser, out, Lib::Aux::TypeSwitch<T>::Store(object, *this)
        );
    }

    template<class TParser, class Object>
    std::vector<uint8_t> Tree::store (const Object& object, TParser&& parser)
    {
        std::vector<uint8_t> vec;
        store(object, vec, parser);
        return vec;
    }

    template<class TParser, class Object>
    void Tree::restore(Object& object, Lib::In::Source source, TParser&& parser)
    {
        this->read_source(parser, source, Lib::Aux::TypeSwitch<Object>::Restore(*this, object));
    }

    template<class Object, class TParser>
    Object Tree::restore(Lib::In::Source source, TParser&& parser)
    {
        auto object = Ctor<Object>::Create();
        this->restore(object, source, parser);

        return object;
    }

    template<class TParser, class Object>
    void Tree::restore(Object& object, const uint8_t* data, size_t len, TParser&& parser)
    {
        this->restore(object, Lib::In::Source(data, len), parser);
    }

    template<class Object, class TParser>
    Object Tree::restore(const uint8_t* data, size_t len, TParser&& parser)
    {
        return this->restore<Object>(Lib::In::Source(data, len), parser);
    }

    template<class T>
    void Tree::load_object(const T& type)
    {
        this->clear();
        Lib::Aux::TypeSwitch<T>::Insert(type, *this);
    }
}

#endif
