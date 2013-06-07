#ifndef SRL_PASRERXML_H
#define SRL_PARSERXML_H

#include "Common.h"
#include "Parser.h"
#include "Blocks.h"
#include "Hash.h"
#include "Type.h"
#include "Heap.h"

namespace Srl {

    class PXml : public Parser {

    public :
        PXml(bool insert_whitespace_ = true)
            : insert_whitespace(insert_whitespace_) {  }

        Format get_format() const { return Format::Text; }

        virtual void
        parse_out(const Value& value, const Lib::MemBlock& name, Lib::Out& out) override;
        virtual SourceSeg parse_in(Lib::In& source) override;

    private :
        bool insert_whitespace,
             document_parsed = false;

        size_t scope_depth  = 0,
               tag_index    = 0;

        struct XmlTag  {
            XmlTag(const Lib::MemBlock& name_, Type type_, size_t parent_tag_,
                   const Lib::MemBlock& data_ )
                : parent_tag(parent_tag_), type(type_), name(name_), data(data_) { }

            size_t n_child_tags           = 0;
            size_t child_name_hash        = 0;
            bool   child_names_consistent = true;

            size_t parent_tag;
            Type   type;

            Lib::MemBlock name;
            Lib::MemBlock data;

            std::string name_string();
        };

        Lib::Heap            buffer;
        std::vector<XmlTag*> tags;
        std::vector<uint8_t> escape_buffer;

        XmlTag& create_tag (const Lib::MemBlock& name, Type type, size_t parent_tag,
                            const Lib::MemBlock& data = Lib::MemBlock(nullptr, 0));

        Lib::MemBlock read_content (Lib::In& source);
        void parse_document     (Lib::In& source);
        void process_tag_open   (const Lib::MemBlock& block, const Lib::MemBlock& content = Lib::MemBlock());
        void process_tag_close  (const Lib::MemBlock& closing_block = Lib::MemBlock());
        void process_scope      (XmlTag& scope, const Lib::MemBlock& closing_tag);
        bool scope_is_container (XmlTag& scope);
        void read_attributes    (Lib::MemBlock& name, Lib::In& source, bool& out_closed);

        XmlTag& current_tag();
    };
}

#endif
