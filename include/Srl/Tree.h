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
        Tree& operator= (Tree&& g);
        Tree(Tree&& g);

        Tree(const String& name = "")
            : env(new Lib::Environment(*this)),
              root_node(&env->create_node(Type::Object, name)->field) { }

        template<class T>
        static Tree From_Type(const T& type, const std::string name = "");

        template<class TParser>
        static Tree From_Source (const std::vector<uint8_t>& source, const TParser& parser = TParser());

        template<class TParser>
        static Tree From_Source (const char* source, size_t source_size, const TParser& parser = TParser());

        template<class TParser>
        static Tree From_Source (const uint8_t* source, size_t source_size, const TParser& parser = TParser());

        template<class TParser>
        static Tree From_Source (std::istream& stream, const TParser& parser = TParser());

        template<class TParser>
        std::vector<uint8_t> to_source(const TParser& parser = TParser());

        template<class TParser>
        void to_source(std::ostream& stream, const TParser& parser = TParser());

        Node& root();

    private:
        std::unique_ptr<Lib::Environment> env;

        Node*     root_node;

        void write (const Value& value, const String& name);
        void write (Type type, Parser& parser, Lib::Out& out, const std::function<void()>& store_switch);

        void set_output (Parser& parser, Lib::Out& out);

        void write_conv (const Value& value, const String& name, Parser& parser);
        Value conv_type (const Value& value);

        void read_source (Parser& parser, Lib::In& source);
        void read_source (Parser& parser, Lib::In& source, const std::function<void()>& restore_switch);

        template<class TParser>
        static Tree From_Source (Lib::In& source, const TParser& parser);

        template<class TParser, class Object>
        friend void Store(const Object& object, Lib::Out& out, const TParser& parser, const std::string& name);

        template<class Object, class TParser>
        friend void Restore(Object& object, Lib::In& source, const TParser& parser);
    };
}

#endif
