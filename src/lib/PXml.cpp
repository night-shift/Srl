#include "Srl/Srl.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

namespace {

    const uint8_t str_name[] = "value", str_name_root[] = "tree";
    const MemBlock empty_name(str_name, sizeof(str_name)),
                   empty_name_root(str_name_root, sizeof(str_name_root));

    const std::string exception_message = "Unable to parse XML document. Document malformed.";

    const function<void()> error = [] {
        throw Exception(exception_message);
    };

    template<class... chars>
    constexpr array<const char, sizeof...(chars) + 2> ar_sub(chars... chr)
    {
        return array<const char, sizeof...(chars) + 2> { { '&', chr..., ';' } };
    }

    template<class... chars>
    constexpr array<const char, sizeof...(chars)> ar(chars... chr)
    {
        return array<const char, sizeof...(chars)> { { chr...} };
    }

    void write_escape(const MemBlock& str, Out& out)
    {
       out.write_substitute(str,
            '&', ar_sub('a','m','p'), '<', ar_sub('l','t'), '>', ar_sub('g','t'),
            '\"', ar_sub('q','o','u','t'), '\'', ar_sub('a','p','o','s')
       );
    }

    MemBlock read_escape(uint8_t delimiter, In& in, vector<uint8_t>& buffer)
    {
        auto len = in.read_substitue(error, delimiter, buffer,
            '&', ar_sub('a','m','p'), '<', ar_sub('l','t'), '>', ar_sub('g','t'),
            '\"', ar_sub('q','o','u','t'), '\'', ar_sub('a','p','o','s')
        );

        return MemBlock(buffer.data(), len);
    }

    void insert_tag(Out& out, const MemBlock& name, bool end_tag)
    {
        out.write('<');
        if(end_tag) {
            out.write('/');
        }
        out.write(name);
        out.write('>');
    }

    void insert_inner_value(Out& out, const Value& value, const MemBlock& name)
    {
        insert_tag(out, name, false);
        write_escape({ value.data(), value.size() }, out);
        insert_tag(out, name, true);
    }

    bool test_name_tags(const MemBlock& opening_tag, const MemBlock& closing_tag)
    {
        auto trimmed = closing_tag;
        Tools::trim_space(trimmed);

        /* empty closing tags are allowed */
        if(trimmed.size == 0U) {
            return true;
        }

        return opening_tag.size == trimmed.size &&
               memcmp(opening_tag.ptr, trimmed.ptr, trimmed.size) == 0;
    }
}

/* Parse out ****************************************************/

void PXml::write(const Value& value, const MemBlock& name, Out& out)
{
    auto type = value.type();

    if(!this->skip_whitespace) {
        out.write_times(this->scope_depth - (type == Type::Scope_End ? 1 : 0), '\t');
    }

    auto name_block = name.size > 0 ? name
           : this->scope_depth == 0 || (this->scope_depth == 1 && type == Type::Scope_End)
           ? empty_name_root : empty_name;

    if(TpTools::is_scope(type) || type == Type::Scope_End) {
        insert_tag(out, name_block, type == Type::Scope_End);
        this->scope_depth += type == Type::Scope_End ? -1 : 1;

   } else {
        insert_inner_value(out, value, name_block);
    }

    if(!this->skip_whitespace) {
        out.write('\n');
    }
}

/* Parse in ****************************************************/

string PXml::XmlTag::name_string()
{
    return this->name.size < 1 ? ""
        : string((const char*)this->name.ptr, this->name.size);
}

PXml::XmlTag& PXml::current_tag()
{
    return *this->tags[this->tag_index];
}

pair<Lib::MemBlock, Value> PXml::read(In& source)
{
    /* Completely parse the document in one go and afterwards 'feed' the caller on
     * request with the results. The document is parsed after the root is closed. */
    if(!this->document_parsed) {
        this->parse_document(source);
    }

    if(this->tag_index >= this->tags.size()) {
        error();
    }

    auto& tag = *this->tags[this->tag_index++];

    if(tag.type == Type::Scope_End || TpTools::is_scope(tag.type)) {

        return { tag.name, Value(tag.type) };

    } else {

        return { tag.name, Value(tag.data, Encoding::UTF8) };
    }
}

void PXml::parse_document(Lib::In& source)
{
    while(!this->document_parsed) {

        source.move_until(error, '<');
        source.move(1, error);

        if(*(source.pointer())== '!' || *(source.pointer()) == '?') {
            source.move_until(error, ar('-', '-', '>'), ar('?', '>'), ar(']','>'));
            continue;
        }

        auto tag = source.read_block_until(error, '=', '>');
        tag = source.is_streaming() ? Aux::copy(this->data_buffer, tag) : tag;

        if(tag.size < 1) {
            error();
        }

        if(*tag.ptr == '/') {
            this->process_tag_close(MemBlock(tag.ptr + 1, tag.size - 1));
            continue;
        }

        bool closed = false;
        if(*source.pointer() == '=') {
            this->read_attributes(tag, source, closed);

        } else {

            if(*(tag.ptr + tag.size - 1) == '/') {
                tag.size--;
                closed = true;
            }
            Tools::trim_space(tag);
            this->process_tag_open(tag);
        }

        if(closed) {
            this->process_tag_close();
            continue;
        }

        source.move(1, error);
        source.skip_space(error);

        MemBlock content = this->read_content(source);

        if(content.size > 0) {
            if(this->current_tag().n_child_tags > 0) {
                /* tag has attributes and inner value */
                this->process_tag_open(MemBlock(), content);
                this->process_tag_close();

            } else {
                this->current_tag().data = content;
            }
       }
    }
}

Lib::MemBlock PXml::read_content(Lib::In& source)
{
    if(source.is_at_token(ar('<','!'))) {
        source.move_until(error, ar('-','-','>'));
        source.move(3, error);
    }

    if(source.is_at_token(ar('<','!','[','C','D','A','T','A','['))) {
        source.move(9, error);
        auto block = source.read_block_until(error, ar(']', ']', '>'));

        return source.is_streaming() ? Aux::copy(this->data_buffer, block) : block;

    } else {
        auto content = read_escape('<', source, this->escape_buffer);
        Tools::trim_space(content);

        return Aux::copy(this->data_buffer, content);
    }
}

void PXml::process_tag_open(const MemBlock& block, const MemBlock& content)
{
    if(this->tags.size() < 1) {
        /* root element */
        this->create_tag(block, Type::Object, 0);
        this->tag_index = 0;

    } else {
        this->create_tag(block, Type::Null, this->tag_index, content);
        this->tag_index = this->tags.size() - 1;
    }
}

void PXml::process_tag_close(const MemBlock& block)
{
    if(this->tags.size() < 1) {
        error();
    }

    auto& closing = this->current_tag();
    this->process_scope(closing, block);

    /* root tag closed -> document is parsed */
    if(this->tag_index == 0) {
        this->document_parsed = true;
        return;
    }

    this->tag_index = closing.parent_tag;
    auto& parent = *this->tags[this->tag_index];
    auto name_hash = murmur_hash2(closing.name.ptr, closing.name.size);

    /* guess: it's a container if all child tags have the same name */
    if(parent.n_child_tags < 1) {
        parent.child_name_hash = name_hash;
    } else {
        bool consistent =
            parent.child_names_consistent &&
            parent.child_name_hash == name_hash;

        parent.child_names_consistent = consistent;
    }

    parent.n_child_tags++;
}

void PXml::process_scope(XmlTag& tag, const MemBlock& closing_tag)
{
    bool same_tags = test_name_tags(tag.name, closing_tag);
    if(!same_tags) {
        throw Exception(exception_message + " Start tag does not match end tag. In <"
                        + tag.name_string() + ">.");
    }

    tag.type = tag.n_child_tags > 0 || this->tag_index == 0
        ? tag.child_names_consistent && tag.n_child_tags > 1 ? Type::Array : Type::Object
        : closing_tag.size < 1  && tag.data.size < 1 ? Type::Array : Type::String;

    if(TpTools::is_scope(tag.type)) {
        this->create_tag(tag.name, Type::Scope_End, this->tag_index);
    }
}

void PXml::read_attributes(MemBlock& tag, In& source, bool& out_closed)
{
    size_t name_len = 0U;
    while(!Tools::is_space(*(tag.ptr + name_len))) {
        name_len++;
        if(name_len + 1 > tag.size) {
            error();
        }
    }

    this->process_tag_open(MemBlock(tag.ptr, name_len));

    MemBlock first(tag.ptr + name_len + 1, tag.size - name_len - 1);
    Tools::trim_space(first);

    this->process_tag_open(first);

    const auto trim_save = [this, &source](MemBlock& block, bool save) {
        Tools::trim_space(block);
        return save ? Aux::copy(this->data_buffer, block) : block;
    };

    while(true) {
        source.move_until(error, '\"');
        source.move(1, error);
        auto value = read_escape('\"', source, this->escape_buffer);

        this->current_tag().data = trim_save(value, true);
        this->process_tag_close(this->current_tag().name);

        source.move(1, error);

        auto name = source.read_block_until(error, '=', '>', '/');

        if(*source.pointer() != '=') {
            if(*source.pointer() == '/') {
                out_closed = true;
                source.move(1, error);
            }
            break;
        }

        name = trim_save(name, source.is_streaming());
        this->process_tag_open(name);
    }
}

PXml::XmlTag& PXml::create_tag(const MemBlock& name, Type type, size_t parent_tag, const MemBlock& data)
{
    auto* tag = this->tag_buffer.create(name, type, parent_tag, data);
    this->tags.push_back(tag);

    return *tag;
}
