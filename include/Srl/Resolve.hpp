#ifndef SRL_RESOLVE_HPP
#define SRL_RESOLVE_HPP

#include "Resolve.h"
#include "Node.h"
#include "Value.h"
#include "Context.h"
#include "Exception.h"
#include "String.h"
#include "BitWrap.h"
#include "Tools.h"
#include "Cast.hpp"
#include "TpTools.hpp"

namespace Srl { namespace Lib {

    namespace Aux {

        const char Chr_Key[] = "key", Chr_Value[] = "value", Chr_First[] = "first", Chr_Second[] = "second";

        const String Str_Key(Chr_Key),     Str_First (Chr_First),
                     Str_Value(Chr_Value), Str_Second(Chr_Second),
                     Str_Empty;

        template<class ID>
        void throw_error(const std::string& msg, const ID& field_id);

        template<class ID>
        void check_type(Type expected, Type got, const ID& field_id);

        template<class ID>
        void check_type_value(Type got, const ID& field_id);

        template<class ID>
        void check_type_scope(Type got, const ID& field_id);

        template<class ID>
        void check_size(size_t required_size, size_t actual_size, const ID& field_id);

        template<class TChar, class ID>
        void copy_string(TChar* dst, size_t size, const Value& value, const ID& id);
    }

    template<class TString> std::pair<Encoding, MemBlock> wrap_string(const TString& str)
    {
        static_assert(is_character_array<TString>::value
                      || is_character<TString>::value
                      || is_basic_string<TString>::value,

        "Srl error. Invalid string type. Valid types are either character arrays, basic_string<T> types or single characters."

        );
        return Switch<TString>::Wrap(str);
    }

    template<class TChar>
    Encoding get_encoding()
    {
        static_assert(is_character<TChar>::value, "Srl error. Not a character type.");

        return sizeof(TChar) == 1 ? Encoding::UTF8
             : sizeof(TChar) == 2 ? Encoding::UTF16
             : sizeof(TChar) == 4 ? Encoding::UTF32
             : Encoding::Unknown;
    }

    template<class T>
    struct Switch<std::basic_string<T>> {
        static const Type type = Type::String;

        static void Insert(const std::basic_string<T>& str, Node& node, const String& name)
        {
            auto pair = Wrap(str);
            node.insert_value(Value(pair.second, pair.first), name);
        }

        static std::pair<Encoding, MemBlock> Wrap(const std::basic_string<T>& str)
        {
            return std::make_pair (
                get_encoding<T>(),
                MemBlock(reinterpret_cast<const uint8_t*>(str.data()), str.size() * sizeof(T))
            );
        }

        template<class ID = String>
        static void Paste(std::basic_string<T>& str, const Value& value, const ID& id = Aux::Str_Empty)
        {
            auto val_type = value.type();

            if(val_type == Type::Null) {
                return;
            }
            Aux::check_type_value(val_type, id);

            if(val_type == Type::String) {
                str = String(value.data(), value.size(), value.encoding()).unwrap<T>();

            } else {
                /* convert literal to string */
                Aux::check_size(TpTools::get_size(val_type), value.size(), id);

                auto tmp_string = Tools::type_to_string(val_type, value.data());
                str = String(tmp_string).unwrap<T>();
            }
        }
    };

    /* single characters are also strings */
    template<class T>
    struct Switch<T, typename std::enable_if<is_character<T>::value>::type> {
        static const Type type = Type::String;

        static void Insert(const T& c, Node& node, const String& name)
        {
            auto pair = Wrap(c);
            node.insert_value(Value(pair.second, pair.first), name);
        }

        static std::pair<Encoding, MemBlock> Wrap(const T& c)
        {
            return std::make_pair(
                get_encoding<T>(), MemBlock(reinterpret_cast<const uint8_t*>(&c), sizeof(T))
            );
        }

        template<class ID = String>
        static void Paste(T& c, const Value& value, const ID& id = Aux::Str_Empty)
        {
            Aux::copy_string(&c, sizeof(T), value, id);
        }
    };

    /* intercept character arrays */
    template<class T>
    struct Switch<T, typename std::enable_if<is_character_array<T>::value>::type> {

        static const Type type = Type::String;

        static const size_t str_size = (std::extent<T>::value - 1) * sizeof(typename array_type<T>::type);

        static void Insert(const T& str, Node& node, const String& name)
        {
            assert(str[std::extent<T>::value - 1] == 0);

            auto pair = Wrap(str);
            node.insert_value(Value(pair.second, pair.first), name);
        }

        static std::pair<Encoding, MemBlock> Wrap(const T& str)
        {
            return std::make_pair(
                get_encoding<typename array_type<T>::type>(),
                MemBlock(reinterpret_cast<const uint8_t*>(str), str_size)
            );
        }

        template<class ID = String>
        static void Paste(T& str, const Value& value, const ID& id = Aux::Str_Empty)
        {
            Aux::copy_string(&str, str_size, value, id);
        }

    };

    /* character arrays which decayed to pointers somewhere along the way,
     * usually through std::initializer_list, so no paste function provided */
    template<class T>
    struct Switch<T, typename std::enable_if<is_cstr_pointer<T>::value>::type> {

        static void Insert(const T& pointer, Node& node, const String& name)
        {
            auto pair = Wrap(pointer);
            node.insert_value(Value(pair.second, pair.first), name);
        }

        static std::pair<Encoding, MemBlock> Wrap(const T& pointer)
        {
            size_t size = 0;
            while(*(pointer + size) != 0) { size++; }

            return std::make_pair(
                get_encoding<typename std::remove_pointer<T>::type>,
                MemBlock(reinterpret_cast<const uint8_t*>(pointer), size)
            );
        }
    };

    /* integer / floating point / bool types */
    template<class T>
    struct Switch<T, typename std::enable_if<is_numeric<T>::value ||
                     std::is_same<T, std::nullptr_t>::value>::type> {

        static const Type type = TpTools::SrlType<T>::type;

        static_assert(type == Type::Null || TpTools::TypeSize<type>::size == sizeof(T),
            "Srl error. Srl type and underlying type size mismatch."
        );

        static void Insert(const T& o, Node& node, const String& name)
        {
            Value value({ reinterpret_cast<const uint8_t*>(&o), sizeof(T) }, type);
            node.insert_value(value, name);
        }

        template<class ID = String>
        static void Paste(T& o, const Value& value, const ID& id = Aux::Str_Empty)
        {
            auto val_type = value.type();

            Aux::check_type_value(val_type, id);

            if(val_type != Type::String) {
                assert(value.size() <= TpTools::Literal_Max_Size && value.size() == TpTools::get_size(val_type));

                Paste_Type(o, value.type(), value.data(), id);

            } else {
                Paste_String(o, value);
            }
        }

        template<class ID = String>
        static void Paste_String(T& o, const Value& value, const ID& id = Aux::Str_Empty)
        {
            String wrap(value.data(), value.size(), value.encoding());
            auto conversion = Tools::string_to_type(wrap);

            if(!conversion.success) {
                auto msg = "Cannot convert string \'"
                    + wrap.unwrap<char>(false) + "\' to numeric type " + TpTools::get_name(type) + ".";
                Aux::throw_error(msg, id);
            }

            Paste_Type(o, conversion.type, (const uint8_t*)&conversion.int_value, id);
        }

        template<class ID = String>
        static void Paste_Type(T& o, Type src_type, const uint8_t* src_mem, const ID& id = Aux::Str_Empty)
        {
            if(src_type == type) {
                o = Read_Cast<T>(src_mem);
                return;
            }

            auto success = TpTools::paste_type(o, src_type, src_mem);
            if(!success) {
                auto msg = "Cannot cast type " + TpTools::get_name(src_type)
                           + " to " + TpTools::get_name(type) + ". Overflow detected.";
                Aux::throw_error(msg, id);
            }
        }
    };

    /* avoid that can of worms for now */
    template<> struct Switch<long double> {
        static const Type type = Switch<double>::type;

        static void Insert(const long double& ld, Node& node, const String& name)
        {
            Switch<double>::Insert((double)ld, node, name);
        }

        template<class ID = String>
        static void Paste(long double& ld, const Value& value, const ID& id = Aux::Str_Empty)
        {
            double d;
            Switch<double>::Paste(d, value, id);
            ld = (long double)d;
        }
    };

    /* Classes / structs which have a srl_resolve method */
    template<class T>
    struct Switch<T, typename std::enable_if<has_resolve_method<T>::value>::type> {
        static const Type type = Type::Object;

        static void Insert(const T& o, Node& node, const String& name)
        {
            node.open_scope(&Insert, type, name, o);
        }

        static void Insert(Node& node, const T& o)
        {
            Context<Mode::Insert> ctx(node);
            /* ugly const-cast, but necessary for making a single resolve function possible */
            const_cast<T*>(&o)->srl_resolve(ctx);
        }

        template<class ID = String>
        static void Paste(T& o, const Node& node, const ID& id = Aux::Str_Empty)
        {
            Aux::check_type_scope(node.type(), id);

            Context<Mode::Paste> ctx(node);
            o.srl_resolve(ctx);
        }
    };

    /* stl-like container types */
    template<class T> struct
    Switch<T, typename std::enable_if<is_container<T>::value>::type> {
        static const Type type = Type::Container;
        typedef typename T::value_type E;

        static void Insert(const T& container, Node& node, const String& name)
        {
            node.open_scope(&Insert, type, name, container);
        }

        static void Insert(Node& node, const T& container)
        {
            for(auto& e : container) {
                ElemSwitch<T, E>::Extract(e, node);
            }
        }

        template<class ID = String>
        static void Paste(T& c, const Node& node, const ID& id = Aux::Str_Empty)
        {
            Aux::check_type_scope(node.type(), id);

            T new_cont;
            auto count = node.num_nodes() < 1 ? node.num_values() : node.num_nodes();

            for(auto i = 0U; i < count; i++) {
                ElemSwitch<T, E>::Insert(new_cont, node, i);
            }

            c = std::move(new_cont);
        }

        template<class Cont, class Elem, typename = void> struct ElemSwitch {
            static void Extract(const Elem& e, Node& node) {
                Switch<Elem>::Insert(e, node, Aux::Str_Empty);
            }
            static void Insert(Cont& c, const Node& node, size_t index) {
                auto itr = c.insert(c.end(), Ctor<Elem>::Create());
                node.paste_field(index, *itr);
            }
        };

        /* containers with immutable elements */
        template<class Cont, class Elem> struct ElemSwitch<Cont, Elem, typename std::enable_if<
            std::is_same<typename Cont::const_iterator, typename Cont::iterator>::value>::type> {
            typedef typename std::remove_const<Elem>::type ElemNC;

            static void Extract(const Elem& e, Node& node) {
                Switch<Elem>::Insert(e, node, Aux::Str_Empty);
            }
            static void Insert(Cont& c, const Node& node, size_t index) {
                auto elem = Ctor<ElemNC>::Create();
                node.paste_field(index, elem);
                c.insert(c.end(), std::move(elem));
            }
        };

        /* special care for map-like containers */
        template<class Cont, typename Key, typename Value> struct ElemSwitch<Cont, std::pair<Key,Value>> {
            typedef typename std::remove_const<Key>::type KeyNC;

            static void Extract(const std::pair<Key,Value>& p, Node& node) {
                Switch<std::pair<KeyNC, Value>>::Insert_Pair(p, node, Aux::Str_Empty, Aux::Str_Key, Aux::Str_Value);
            }

            static void Insert(Cont& c, const Node& node, size_t index) {
                auto* pair_node = node.node(index);

                auto key = Ctor<KeyNC>::Create();
                pair_node->paste_field(Aux::Str_Key, key);

                auto itr = c.insert(c.end(), { key, Ctor<Value>::Create() });
                pair_node->paste_field(Aux::Str_Value, itr->second);
            }
        };
    };

    /* Arrays are treated like containers */
    template<class T>
    struct Switch<T, typename std::enable_if<std::is_array<T>::value && !is_character_array<T>::value>::type> {
        static const Type type = Type::Container;
        typedef typename array_type<T>::type E;

        static void Insert(const T& ar, Node& node, const String& name)
        {
            node.open_scope(&Insert, type, name, ar);
        }

        static void Insert(Node& node, const T& ar)
        {
            for(auto& e : ar) {
                Switch<E>::Insert(e, node, Aux::Str_Empty);
            }
        }

        template<class ID = String>
        static void Paste(T& ar, const Node& node, const ID& id = Aux::Str_Empty)
        {
            auto count = TpTools::is_scope(Switch<E>::type) ? node.num_nodes() : node.num_values();

            Aux::check_size(std::extent<T>::value, count, id);
            Aux::check_type_scope(node.type(), id);

            for(auto i = 0U; i < count; i++) {
                node.paste_field(i, ar[i]);
            }
        }
    };

    template<typename F, typename S> struct Switch<std::pair<F,S>> {
        static const Type type = Type::Object;

        static void Insert(const std::pair<F,S>& p, Node& node, const String& name)
        {
            Insert_Pair(p, node, name, Aux::Str_First, Aux::Str_Second);
        }

        static void Insert_Pair(const std::pair<F,S>& p, Node& node, const String& node_name,
                                const String& first_id, const String& second_id)
        {
            node.open_scope(&Insert, Type::Object, node_name, p, first_id, second_id);
        }

        static void Insert(Node& node, const std::pair<F,S>& pair,
                           const String& first_id, const String& second_id)
        {
            Switch<F>::Insert(pair.first,  node, first_id);
            Switch<S>::Insert(pair.second, node, second_id);
        }

        template<class ID = String>
        static void Paste(std::pair<F,S>& p, const Node& node, const ID& id = Aux::Str_Empty)
        {
            Aux::check_type(Type::Object, node.type(), id);

            node.paste_field(Aux::Str_First, p.first);
            node.paste_field(Aux::Str_Second, p.second);
        }
    };

    template<> struct Switch<BitWrap> {
        static const Type type = Type::Binary;

        static void Insert(const BitWrap& wrap, Node& node, const String& name)
        {
            if(wrap.data.size > 0 && wrap.data.ptr == nullptr) {
                auto msg = "Unable to insert value"
                    + (name.size() < 1 ? "" : " " + name.unwrap<char>(false))
                    + ". Data pointer in BitWrap is null.";
                throw Exception(msg);
            }
            Value value(wrap.data, Type::Binary);
            node.insert_value(value, name);
        }

        template<class ID = String>
        static void Paste(BitWrap& wrap, const Value& value, const ID& id = Aux::Str_Empty)
        {
            if(value.type() != Type::String) {
                Aux::check_type(Type::Binary, value.type(), id);
            }

            MemBlock mem_block(value.data(), value.size());

            auto size = value.type() == Type::String
                ? Tools::get_base64_decoded_size((const char*)mem_block.ptr, mem_block.size)
                : mem_block.size;

            if(!wrap.alloc) {
                Aux::throw_error("No alloc function set in BitWrap.", id);
            }
            uint8_t* data_dst = wrap.alloc(size);

            if(data_dst == nullptr) {
                Aux::throw_error("Nullptr received form alloc() in BitWrap.", id);
            }

            if(value.type() == Type::String) {
                Tools::base64_to_bytes(data_dst, size, (const char*)mem_block.ptr, mem_block.size);
            } else {
                memcpy(data_dst, mem_block.ptr, size);
            }
        }
    };

    namespace Aux {
        /* auxiliary functions */
        template<class TChar, class ID>
        void copy_string(TChar* dst, size_t size, const Value& value, const ID& id)
        {
            check_type(Type::String, value.type(), id);

            auto encoding = get_encoding<TChar>();

            if(value.encoding() == encoding) {
                check_size(size, value.size(), id);
                memcpy(dst, value.data(), size);

            } else {
                std::vector<uint8_t> buffer;
                String wrap(value.data(), value.size(), value.encoding());

                auto conv_size = Tools::convert_charset(encoding, wrap, buffer, true);

                check_size(size, conv_size, id);
                memcpy(dst, &buffer[0], size);
            }
        }

        template<class ID>
        void throw_error(const std::string& msg, const ID& field_id)
        {
            auto message = "Unable to set field at pos : "  + std::to_string(field_id) + ". " + msg;
            throw Exception(message);
        }

        template<>
        inline void throw_error<String>(const std::string& msg, const String& field_id)
        {
            auto message = "Unable to set field" +
                (field_id.size() > 0 ? " " + field_id.unwrap<char>(false) : "") + ". " + msg;
            throw Exception(message);
        }

        template<class ID>
        void check_type(Type expected, Type got, const ID& field_id)
        {
            if(expected != got) {
                throw_error("Type mismatch, expected: " + TpTools::get_name(expected)
                            + ", got: " + TpTools::get_name(got) + ".", field_id);
            }
        }

        template<class ID>
        void check_type_value(Type got, const ID& field_id)
        {
            if(got != Type::String && !TpTools::is_literal(got)) {
                throw_error("Type mismatch, expected: string or literal, got: "
                            + TpTools::get_name(got) + ".", field_id);
            }
        }

        template<class ID>
        void check_type_scope(Type got, const ID& field_id)
        {
            if(!TpTools::is_scope(got)) {
                throw_error("Type mismatch, expected: " + TpTools::get_name(Type::Object)
                            + " or " + TpTools::get_name(Type::Container) + ", got: "
                            + TpTools::get_name(got) + ".", field_id);
            }
        }

        template<class ID>
        void check_size(size_t required_size, size_t actual_size, const ID& field_id)
        {
            if(actual_size != required_size) {
                throw_error("Size mismatch.", field_id);
            }
        }
} } }

#endif
