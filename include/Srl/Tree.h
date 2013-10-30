#ifndef SRL_TREE_H
#define SRL_TREE_H

#include "Environment.h"
#include "Out.h"

#include <memory>

namespace Srl {

    class Node;

    class Tree {

    friend class Node;

    public:
        Tree(const String& name = "");

        Tree& operator= (Tree&& g);
        Tree(Tree&& g);

        Node& root();

        void clear();

        template<class T>
        void load_object(const T& type);

        template<class TParser>
        void load_source (Lib::In::Source source, TParser&& parser = TParser());

        template<class TParser>
        void load_source (const uint8_t* data, size_t data_len, TParser&& parser = TParser());

        template<class TParser>
        void load_source (const char* data, size_t data_len,TParser&& parser = TParser());

        template<class TParser>
        std::vector<uint8_t> to_source (TParser&& parser = TParser());

        template<class TParser>
        void to_source (Lib::Out::Source source, TParser&& parser = TParser());

        template<class TParser, class Object>
        void store (const Object& object, Lib::Out::Source out, TParser&& parser = TParser());

        template<class TParser, class Object>
        std::vector<uint8_t> store (const Object& object, TParser&& parser = TParser());

        template<class TParser, class Object>
        void restore(Object& object, Lib::In::Source source, TParser&& parser = TParser());

        template<class Object, class TParser>
        Object restore(Lib::In::Source source, TParser&& parser = TParser());

        template<class TParser, class Object>
        void restore(Object& object, const uint8_t* data, size_t len, TParser&& parser = TParser());

        template<class Object, class TParser>
        Object restore(const uint8_t* data, size_t len, TParser&& parser = TParser());

        template<class TParser, class... Items>
        void pack(Lib::Out::Source out, TParser&& parser, const Items&... items);

        template<class TParser, class... Items>
        void unpack(Lib::In::Source in, TParser&& parser, Items&... items);

    private:
        std::unique_ptr<Lib::Environment> env;

        Node* root_node;

        void to_source(Type type, Parser& parser, Lib::Out::Source out, const std::function<void()>& store_switch);

        void read_source (Parser& parser, Lib::In::Source source);
        void read_source (Parser& parser, Lib::In::Source source, const std::function<void()>& restore_switch);

        void prologue_in(Parser& parser, Lib::In::Source& source);

        template<class Object>
        friend void Restore(Object& object, Lib::In::Source source, Parser& parser);
    };
}

#endif
