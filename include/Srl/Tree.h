#ifndef SRL_TREE_H
#define SRL_TREE_H

#include "Environment.h"
#include "Node.h"
#include "Out.h"
#include "Util.h"

#include <memory>

namespace Srl {

    class Tree {

    friend class Node;

    public:
        Tree(const String& name = "");

        Tree& operator= (Tree&& g);
        Tree(Tree&& g);


        template<class T>
        void load_object(const T& type);

        template<class TParser>
        void load_source (const std::vector<uint8_t>& source, TParser&& parser = TParser());

        template<class TParser>
        void load_source (const char* source, size_t source_size, TParser&& parser = TParser());

        template<class TParser>
        void load_source (const uint8_t* source, size_t source_size, TParser&& parser = TParser());

        template<class TParser>
        void load_source (std::istream& stream, TParser&& parser = TParser());

        template<class TParser>
        std::vector<uint8_t> to_source(TParser&& parser = TParser());

        template<class TParser>
        void to_source(std::ostream& stream, TParser&& parser = TParser());

        Node& root();

        void clear();

    private:
        std::unique_ptr<Lib::Environment> env;

        Node* root_node;

        void write (const Value& value, const String& name);
        void write (Type type, Parser& parser, Lib::Out& out, const std::function<void()>& store_switch);

        void set_output (Parser& parser, Lib::Out& out);

        void write_conv (const Value& value, const String& name, Parser& parser);
        Value conv_type (const Value& value);

        void read_source (Parser& parser, Lib::In& source);
        void read_source (Parser& parser, Lib::In& source, const std::function<void()>& restore_switch);

        template<class TParser>
        void load_source (Lib::In& source, TParser& parser);

        template<class Object>
        friend void Store(const Object& object, Lib::Out& out, Parser& parser);

        template<class Object>
        friend void Restore(Object& object, Lib::In& source, Parser& parser);
    };
}

#endif
