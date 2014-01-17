#ifndef SRL_PXML_H
#define SRL_PXML_H

#include "Parser.h"
#include "Type.h"
#include "Hash.h"
#include "Heap.h"

namespace Srl {

    class PXml : public Parser {

    public :
        PXml(bool compact_ = true)
            : compact(compact_) {  }

        Format get_format() const  { return Format::Text; }

        void set_compact(bool val) { this->compact = val; }

        virtual void
        write(const Value& value, const Lib::MemBlock& name, Lib::Out& out) override;
        virtual std::pair<Lib::MemBlock, Value> read(Lib::In& source) override;
        virtual void clear() override;

    private :
        bool compact,
             document_parsed = false;

        size_t scope_depth = 0,
               tag_index   = 0;

        struct XmlTag  {
            XmlTag(const Lib::MemBlock& name_, Type type_, size_t parent_tag_,
                   const Lib::MemBlock& data_ )
                : parent_tag(parent_tag_), name(name_), data(data_), type(type_) { }

            size_t child_names = 0;
            size_t nchild_tags = 0;
            size_t parent_tag;

            Lib::MemBlock name;
            Lib::MemBlock data;

            Type   type;
            bool   names_consistent = true;

            std::string name_string();
        };

        Lib::Heap            heap;
        std::deque<XmlTag*>  tags;
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

        XmlTag& scope_tag();
    };
}

#endif
