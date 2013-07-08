#ifndef SRL_TREE_H
#define SRL_TREE_H

#include "Storage.h"
#include "Node.h"
#include "Out.h"
#include "Util.h"

namespace Srl {

    class Tree {

    friend class Node;

    public:
        Tree& operator= (Tree&& g);
        Tree(Tree&& g);

        Tree(const String& name = "")
            : root_node(&storage.create_node(*this, Type::Object, name)->field) { }

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

        Node* root();

        void srl_resolve(Context& ctx);

    private:
        Lib::Storage storage;
        Node*        root_node;

        Parser*   temp_parser = nullptr;
        Lib::Out* temp_stream = nullptr;

        void parse_value (const Value& value, const String& name);
        void parse_out (Parser& parser, Lib::Out& out, const std::function<void()>& store_switch);

        void set_output (Parser& parser, Lib::Out& out);

        void parse_out_convert (const Value& value, const String& name, Parser& parser);
        Value convert_type (const Value& value);

        void parse_in (Parser& parser, Lib::In& source, bool just_parse = false);

        template<class TParser>
        static Tree From_Source (Lib::In& source, const TParser& parser);

        template<class TParser, typename Data>
        friend void Lib::Aux::Store(const Data& data, Lib::Out& out, const TParser& parser, const std::string& name);

        template<typename Data, class TParser>
        friend void Restore(Data& data, Lib::In& source, const TParser& parser);
    };
}

#endif
