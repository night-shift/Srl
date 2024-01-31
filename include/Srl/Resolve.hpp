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
#include "TpTools.h"
#include "Registration.h"

namespace Srl { namespace Lib {

    namespace Aux {

        const String str_key("key"),     str_first ("first"),
                     str_value("value"), str_second("second"),
                     str_shared_value("srl_shared_value"), str_shared_key("srl_shared_key"),
                     str_type_id("srl_type_id"), str_empty;

        inline void throw_error(const std::string& msg, size_t field_id);

        inline void throw_error(const std::string& msg, const String& field_id);

        template<class ID>
        void check_type(Type expected, Type got, const ID& field_id);

        template<class ID>
        void check_type_value(Type got, const ID& field_id);

        template<class ID>
        void check_type_scope(Type got, const ID& field_id);

        template<class ID>
        void check_type_string(Type got, const ID& field_id);

        template<class ID>
        void check_size(size_t required_size, size_t actual_size, const ID& field_id);

        template<class ID>
        void check_size_max(size_t required_min_size, size_t actual_size, const ID& field_id);

        template<class TChar, class ID>
        void copy_string(TChar* dst, size_t size, const Value& value, const ID& id);

        template<class T, class ID>
        typename std::enable_if<is_polymorphic<T*>::value, void>::type
        ptr_insert(T* const p, Node& node, const ID& id);

        template<class T, class ID>
        typename std::enable_if<!is_polymorphic<T*>::value, void>::type
        ptr_insert(T* const p, Node& node, const ID& id);
    }

    template<class TString> std::pair<Encoding, MemBlock> wrap_string(const TString& str)
    {
        static_assert(is_char_array<TString>::value
                      || is_char<TString>::value
                      || is_basic_string<TString>::value
                      || is_cstr_pointer<TString>::value,

        "Srl error. Invalid string type. Valid types are either character arrays,\
        basic_string<T> types or single characters."

        );
        return Switch<TString>::Wrap(str);
    }

    template<class TChar>
    Encoding get_encoding()
    {
        static_assert(is_char<TChar>::value, "Srl error. Not a character type.");

        return sizeof(TChar) == 1 ? Encoding::UTF8
             : sizeof(TChar) == 2 ? Encoding::UTF16
             : sizeof(TChar) == 4 ? Encoding::UTF32
             : Encoding::Unknown;
    }

    /* handle std::basic_string<T> types */
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
        static void Paste(std::basic_string<T>& str, const Value& value, const ID& id = Aux::str_empty)
        {
            auto val_type = value.type();

            if(val_type == Type::Null) {
                return;
            }

            if(val_type == Type::String) {
                str = String(value.data(), value.size(), value.encoding()).unwrap<T>();

            } else {
                /* convert literal to string */
                Aux::check_type_value(val_type, id);
                Aux::check_size(TpTools::get_size(val_type), value.size(), id);

                auto tmp_string = Tools::type_to_str(value);
                str = String(tmp_string).unwrap<T>();
            }
        }
    };

    /* single characters are also strings */
    template<class T>
    struct Switch<T, typename std::enable_if<is_char<T>::value>::type> {
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
        static void Paste(T& c, const Value& value, const ID& id = Aux::str_empty)
        {
            Aux::copy_string(&c, sizeof(T), value, id);
        }
    };

    /* intercept character arrays and handle those like strings */
    template<class T>
    struct Switch<T, typename std::enable_if<is_char_array<T>::value>::type> {

        static const Type type = Type::String;

        static const size_t chr_size = sizeof(typename array_type<T>::type);
        static const size_t str_size = (std::extent<T>::value - 1) * chr_size;

        static void Insert(const T& str, Node& node, const String& name)
        {
            auto pair = Wrap(str);
            node.insert_value(Value(pair.second, pair.first), name);
        }

        static std::pair<Encoding, MemBlock> Wrap(const T& str)
        {
            return std::make_pair(
                get_encoding<typename array_type<T>::type>(),
                WrapArray<T>(str)
            );
        }

        template<class C>
        typename std::enable_if<std::extent<C>::value - 1 != 0, MemBlock>::type
        static WrapArray(const T& str)
        {
            size_t size = 0;
            while(size < str_size && str[size]) size++;

            return MemBlock(reinterpret_cast<const uint8_t*>(str), size * chr_size);
        }

        template<class C>
        typename std::enable_if<std::extent<C>::value - 1 == 0, MemBlock>::type
        static WrapArray(const T&)
        {
            return MemBlock();
        }

        template<class ID = String>
        static void Paste(T& str, const Value& value, const ID& id = Aux::str_empty)
        {
            Aux::check_type_string(value.type(), id);

            auto encoding = get_encoding<typename array_type<T>::type>();

            if(value.encoding() == encoding) {
                Aux::check_size_max(str_size, value.size(), id);
                memcpy(str, value.data(), value.size());
                str[value.size() / chr_size] = 0;

            } else {
                std::vector<uint8_t> buffer;
                String wrap(value.data(), value.size(), value.encoding());

                auto conv_size = Tools::conv_charset(encoding, wrap, buffer, true);

                Aux::check_size_max(str_size, conv_size, id);
                memcpy(str, buffer.data(), conv_size);
                str[value.size() / chr_size] = 0;
            }
        }
    };

    /* character arrays which decayed to pointers somewhere along the way,
     * usually through std::initializer_list */
    template<class T>
    struct Switch<T, typename std::enable_if<is_cstr_pointer<T>::value>::type> {

        static const size_t chr_size = sizeof(typename std::remove_pointer<T>::type);
        static const Type type = Type::String;

        static void Insert(const T& pointer, Node& node, const String& name)
        {
            auto pair = Wrap(pointer);
            node.insert_value(Value(pair.second, pair.first), name);
        }

        static std::pair<Encoding, MemBlock> Wrap(const T& pointer)
        {
            size_t size = 0;
            while(pointer[size]) size++;

            return std::make_pair(
                get_encoding<typename std::remove_pointer<T>::type>(),
                MemBlock(reinterpret_cast<const uint8_t*>(pointer), size * chr_size)
            );
        }

        template<class ID = String>
        static void Paste(T&, const Value&, const ID&) { }
    };

    /* srl string abstraction */
    template<> struct Switch<String> {
        static const Type type = Type::String;

        static void Insert(const String& s, Node& node, const String& name)
        {
            node.insert_value(Value({ s.data(), s.size() }, s.encoding()), name);
        }

        template<class ID = String>
        static void Paste(String& s, const Value& value, const ID& id = Aux::str_empty)
        {
            Aux::check_type_string(value.type(), id);
            s = String({ value.data(), value.size() }, value.encoding());
        }
    };

    template<> struct Switch<Union> {

        template<class ID = String>
        static void Paste(Union& u, Node& node, const ID& id = Aux::str_empty)
        {
            if(node.parsed) {
                u = node.get(id);
            } else {
                u = node.consume_item(id);
                node.consume_scope();
            }
        }
    };

    template<> struct Switch<std::nullptr_t> {

        static void Insert(const std::nullptr_t&, Node& node, const String& name)
        {
            node.insert_value(Value(Type::Null), name);
        }
    };

    /* integer / floating point / bool types */
    template<class T>
    struct Switch<T, typename std::enable_if<is_numeric<T>::value>::type> {

        static const Type type = TpTools::SrlType<T>::type;

        static_assert(type == Type::Null || TpTools::TypeSize<type>::size == sizeof(T),
            "Srl error. Srl type and underlying type size mismatch."
        );

        static void Insert(const T& o, Node& node, const String& name)
        {
            Value value(o);
            node.insert_value(value, name);
        }

        template<class ID = String>
        static void Paste(T& o, const Value& value, const ID& id = Aux::str_empty)
        {
            auto val_type = value.type();

            Aux::check_type_value(val_type, id);

            if(val_type != Type::String) {
                Apply(o, value, id);

            } else {
                Apply_String(o, value, id);
            }
        }

        template<class ID = String>
        static void Apply_String(T& o, const Value& value, const ID& id = Aux::str_empty)
        {
            String wrap(value.data(), value.size(), value.encoding());

            auto conv = Tools::str_to_type(wrap);

            if(!conv.first) {
                auto msg = "Cannot convert string \'"
                    + wrap.unwrap(false) + "\' to numeric type " + TpTools::get_name(type) + ".";
                Aux::throw_error(msg, id);
            }

            Apply(o, conv.second, id);
        }

        template<class ID = String>
        static void Apply(T& o, const Value& value, const ID& id = Aux::str_empty)
        {
            auto success = TpTools::apply_type(o, value);

            if(!success) {
                auto msg = "Cannot cast type " + TpTools::get_name(value.type())
                           + " to " + TpTools::get_name(type) + ". Overflow detected.";
                Aux::throw_error(msg, id);
            }
        }
    };

    template<> struct Switch<long double> {
        static const Type type = Switch<double>::type;

        static void Insert(const long double& ld, Node& node, const String& name)
        {
            Switch<double>::Insert((double)ld, node, name);
        }

        template<class ID = String>
        static void Paste(long double& ld, const Value& value, const ID& id = Aux::str_empty)
        {
            double d;
            Switch<double>::Paste(d, value, id);
            ld = (long double)d;
        }
    };

    /* Classes / structs which have srl_resolve methods but no type_id method */
    template<class T>
    struct Switch<T, typename std::enable_if<
        has_resolve_method<T>::value || has_store_method<T>::value || has_restore_method<T>::value>::type> {

        static_assert(
            has_resolve_method<T>::value || (has_store_method<T>::value && has_restore_method<T>::value),
            "Srl error. Classes either need to implement a single srl_resolve method or both "
            "srl_restore and srl_store methods"
        );

        static const Type type = Type::Object;

        static void Insert(const T& o, Node& node, const String& name)
        {
            node.open_scope(&Insert, type, name, o);
        }

        static void Insert(Node& node, const T& o)
        {
            InsertCall<T>(node, o);
        }

        template<class U>
        typename std::enable_if<has_store_method<U>::value, void>::type
        static InsertCall(Node& node, const T& o)
        {
            StoreContext ctx(node);
            o.srl_store(ctx);
        }

        template<class U>
        typename std::enable_if<has_resolve_method<U>::value, void>::type
        static InsertCall(Node& node, const T& o)
        {
            Context ctx(node, Mode::Insert);
            const_cast<T*>(&o)->srl_resolve(ctx);
        }

        template<class ID = String>
        static void Paste(T& o, Node& node, const ID& id = Aux::str_empty)
        {
            Aux::check_type_scope(node.type(), id);

            PasteCall<T>(node, o);
        }

        template<class U>
        typename std::enable_if<has_resolve_method<U>::value, void>::type
        static PasteCall(Node& node, T& o)
        {
            Context ctx(node, Mode::Paste);
            o.srl_resolve(ctx);
        }

        template<class U>
        typename std::enable_if<has_restore_method<U>::value, void>::type
        static PasteCall(Node& node, T& o)
        {
            RestoreContext ctx(node);
            o.srl_restore(ctx);
        }
    };

    /* polymorphic classes / structs with a srl_type_id method */
    template<class T>
    struct Switch<T, typename std::enable_if<is_polymorphic<T>::value>::type> {
        static const Type type = Type::Object;

        static void Insert(const T& o, Node& node, const String& name)
        {
            if(o == nullptr) {
                Aux::throw_error("Trying to serialize object pointed by nullptr.", name);
            }

            node.open_scope(&Insert, type, name, o);
        }

        static void Insert(Node& node, const T& o)
        {
            Context ctx(node, Mode::Insert);
            auto* id = o->srl_type_id().name();
            ctx(Aux::str_type_id, id);

            const_cast<T>(o)->srl_resolve(ctx);
        }

        template<class ID = String>
        static void Paste(T& o, Node& node, const ID& id = Aux::str_empty)
        {
            typedef typename std::remove_pointer<T>::type U;

            Aux::check_type_scope(node.type(), id);

            Context ctx(node, Mode::Paste);
            String type_id;
            ctx(Aux::str_type_id, type_id);

            if(o != nullptr) {
                delete o;
            }

            auto uptr = Lib::registrations()->create<U>(type_id);
            uptr->srl_resolve(ctx);
            o = uptr.release();
        }
    };

    /* stl-like container types */
    template<class T> struct
    Switch<T, typename std::enable_if<is_container<T>::value>::type> {
        static const Type type = Type::Array;
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

        template<class Item>
        static typename std::enable_if<std::is_same<Item, Value>::value, void>::type
        Finish(Item&) { }

        template<class Item>
        static typename std::enable_if<std::is_same<Item, Node>::value, void>::type
        Finish(Item& node) { node.consume_scope(); }

        template<class ID = String>
        static void Paste(T& c, Node& node, const ID& id = Aux::str_empty)
        {
            Aux::check_type_scope(node.type(), id);

            T new_cont;
            size_t count = 0;

            if(node.parsed) {
                for(auto& itm : node.items<E>()) {
                    ElemSwitch<T, E>::Insert(new_cont, itm.field, count++);
                }

            } else {
                while(true) {
                    auto itm = node.consume_item<E>();
                    if(node.parsed) {
                        break;
                    }
                    ElemSwitch<T, E>::Insert(new_cont, itm, count++);
                    Finish(itm);
                }
            }

            c = std::move(new_cont);
        }

        template<class Cont, class Elem, typename = void> struct ElemSwitch {
            static void Extract(const Elem& e, Node& node) {
                Switch<Elem>::Insert(e, node, Aux::str_empty);
            }
            template<class Item>
            static void Insert(Cont& c, Item& itm, size_t index) {
                c.emplace_back(Ctor<Elem>::Create());
                Switch<Elem>::Paste(c.back(), itm, index);
            }
        };

        /* containers with immutable elements */
        template<class Cont, class Elem> struct ElemSwitch<Cont, Elem, typename std::enable_if<
            std::is_same<typename Cont::const_iterator, typename Cont::iterator>::value>::type> {
            typedef typename std::remove_const<Elem>::type ElemNC;

            static void Extract(const Elem& e, Node& node) {
                Switch<Elem>::Insert(e, node, Aux::str_empty);
            }

            template<class Item>
            static void Insert(Cont& c, Item& itm, size_t index) {
                auto elem = Ctor<ElemNC>::Create();
                Switch<Elem>::Paste(elem, itm, index);
                c.insert(c.end(), std::move(elem));
            }
        };

        /* special care for map-like containers */
        template<class Cont, typename Key, typename Value> struct ElemSwitch<Cont, std::pair<Key,Value>> {
            typedef typename std::remove_const<Key>::type KeyNC;

            static void Extract(const std::pair<Key,Value>& p, Node& node) {
                Switch<std::pair<KeyNC, Value>>::InsertPair(p, node, Aux::str_empty, Aux::str_key, Aux::str_value);
            }

            template<class Item>
            static void Insert(Cont& c, Item& itm, size_t) {
                auto key = Ctor<KeyNC>::Create();
                itm.paste_field(Aux::str_key, key);

                auto val = Ctor<Value>::Create();
                itm.paste_field(Aux::str_value, val);

                c.emplace(std::move(key), std::move(val));
            }
        };
    };

    /* arrays */
    template<class T>
    struct Switch<T, typename std::enable_if<std::is_array<T>::value && !is_char_array<T>::value>::type> {
        static const Type type  = Type::Array;
        static const size_t len = std::extent<T>::value;
        typedef typename array_type<T>::type E;

        static void Insert(const T& ar, Node& node, const String& name)
        {
            node.open_scope(&Insert, type, name, ar);
        }

        static void Insert(Node& node, const T& ar)
        {
            for(auto& e : ar) {
                Switch<E>::Insert(e, node, Aux::str_empty);
            }
        }

        template<class ID = String>
        static void Paste(T& ar, Node& node, const ID& id = Aux::str_empty)
        {
            Aux::check_type_scope(node.type(), id);

            size_t count = 0;

            if(node.parsed) {

                Aux::check_size(len, node.items<E>().size(), id);

                for(auto& itm : node.items<E>()) {
                    Switch<E>::Paste(ar[count], itm.field, count);
                    count++;
                }

            } else {
                while(!node.parsed && count < len) {
                    node.paste_field(count, ar[count]);
                    count++;
                }

                Aux::check_size(len, count, id);
            }
        }
    };

    template<class F, class S> struct Switch<std::pair<F,S>> {
        static const Type type = Type::Object;

        static void Insert(const std::pair<F,S>& p, Node& node, const String& name)
        {
            InsertPair(p, node, name, Aux::str_first, Aux::str_second);
        }

        static void InsertPair(const std::pair<F,S>& p, Node& node, const String& node_name,
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
        static void Paste(std::pair<F,S>& p, Node& node, const ID& id = Aux::str_empty)
        {
            Aux::check_type(Type::Object, node.type(), id);

            node.paste_field(Aux::str_first, p.first);
            node.paste_field(Aux::str_second, p.second);
        }
    };

    /* tuples */
    template<class... T> struct Switch<std::tuple<T...>> {
        static const Type type = Type::Array;
        static constexpr size_t Size = sizeof...(T);

        static void Insert(const std::tuple<T...>& tpl, Node& node, const String& name)
        {
            node.open_scope(&Insert<0>, type, name, tpl);
        }

        template<size_t N> static typename std::enable_if<N < Size, void>::type
        Insert(Node& node, const std::tuple<T...>& tpl)
        {
            node.insert(std::get<N>(tpl));
            Insert<N + 1>(node, tpl);
        }

        template<class ID = String>
        static void Paste(std::tuple<T...>& tpl, Node& node, const ID& id = Aux::str_empty)
        {
            Aux::check_type_scope(node.type(), id);

            Context ctx(node, Mode::Paste);
            Paste<0>(ctx, tpl);
        }

        template<size_t N> static typename std::enable_if<N < Size, void>::type
        Paste(Context& ctx, std::tuple<T...>& tpl)
        {
            ctx(std::get<N>(tpl));
            Paste<N + 1>(ctx, tpl);
        }

        template<size_t N> static typename std::enable_if<N >= Size, void>::type
        Paste(Context&, std::tuple<T...>&) { }

        template<size_t N> static typename std::enable_if<N >= Size, void>::type
        Insert(Node&, const std::tuple<T...>&) { }
    };


    /* pointer -> value referenced by pointer will be serialized */
    template<class T>
    struct Switch<T, typename std::enable_if<std::is_pointer<T>::value && !is_polymorphic<T>::value && !is_cstr_pointer<T>::value>::type> {
        typedef typename std::remove_pointer<T>::type E;
        static const Type type = Switch<E>::type;

        static void Insert(const T& p, Node& node, const String& name)
        {
            if(p == nullptr) {
                Switch<std::nullptr_t>::Insert(nullptr, node, name);
                return;
            }
            Aux::ptr_insert<E>(p, node, name);
        }

        template<class Item, class ID = String>
        static void Paste(T& p, Item& item, const ID& id = Aux::str_empty)
        {

            if(p != nullptr) {
                delete p;
            }

            auto val_type = item.type();

            if(val_type == Type::Null) {
                p = nullptr;
                return;
            }

            auto uptr = Ctor<E>::Create_New();
            Switch<E>::Paste(*uptr.get(), item, id);
            p = uptr.release();
        }
    };

    /* unique_ptr */
    template<class T> struct Switch<T, typename std::enable_if<is_unique_ptr<T>::value>::type> {
        typedef typename T::element_type E;
        static const Type type = Switch<E>::type;

        static void Insert(const T& p, Node& node, const String& name)
        {
            auto* ptr = p.get();
            Aux::ptr_insert<E>(ptr, node, name);
        }

        template<class Item, class ID = String>
        static void Paste(T& p, Item& item, const ID& id = Aux::str_empty)
        {
            E* content = nullptr;
            Apply(content, item, id);
            p = T(content);
        }

        template<class C, class Item, class ID>
        static typename std::enable_if<is_polymorphic<C*>::value, void>::type
        Apply(C*& p, Item& item, const ID& id)
        {
            Switch<C*>::Paste(p, item, id);
        }

        template<class C, class Item, class ID>
        static typename std::enable_if<!is_polymorphic<C*>::value, void>::type
        Apply(C*& p, Item& item, const ID& id)
        {
            auto uptr = Ctor<C>::Create_New();
            Switch<C>::Paste(*uptr.get(), item, id);
            p = uptr.release();
        }
    };

    /* shared_ptr */
    template<class T> struct Switch<T, typename std::enable_if<is_shared_ptr<T>::value>::type> {
        typedef typename T::element_type E;
        static const Type type = Type::Object;

        static void Insert(const T& p, Node& node, const String& name)
        {
            if(!p) {
                Switch<std::nullptr_t>::Insert(nullptr, node, name);
                return;
            }
            auto key = node.insert_shared(p.get());
            node.open_scope(&Insert_Key, Type::Object, name, p, !key.first, key.second);
        }

        static void Insert_Key(Node& node, const T& p, const bool& first, const uint64_t& key)
        {
            node.insert_value(key, Aux::str_shared_key);
            if(first) {
                Aux::ptr_insert<E>(p.get(), node, Aux::str_shared_value);
            }
        }

        template<class ID = String>
        static void Paste(T& p, Node& node, const ID& = Aux::str_empty)
        {
            uint64_t key = node.unwrap_field<uint64_t>(Aux::str_shared_key);
            bool inserted_new; std::shared_ptr<void>* sptr;

            std::tie(inserted_new, sptr) = node.find_shared(key, [&p, &node] {
                E* e = nullptr;
                Apply(e, node);
                p = std::shared_ptr<E>(e);
                return p;
            });

            if(!inserted_new) {
                p = std::static_pointer_cast<E>(*sptr);
            }
        }

        template<class C>
        static typename std::enable_if<is_polymorphic<C*>::value, void>::type
        Apply(C*& p, Node& node)
        {
            node.paste_field(Aux::str_shared_value, p);
        }

        template<class C>
        static typename std::enable_if<!is_polymorphic<C*>::value, void>::type
        Apply(C*& p, Node& node)
        {
            auto uptr = Ctor<C>::Create_New();
            node.paste_field(Aux::str_shared_value, *uptr.get());
            p = uptr.release();
        }
    };

    /* weak_ptr */
    template<class T> struct Switch<T, typename std::enable_if<is_weak_ptr<T>::value>::type> {
        typedef typename T::element_type E;
        static const Type type = Type::Object;

        static void Insert(const T& p, Node& node, const String& name)
        {
            auto shared = p.lock();
            Switch<std::shared_ptr<E>>::Insert(shared, node, name);
        }

        template<class ID = String>
        static void Paste(T& p, Node& node, const ID& id = Aux::str_empty)
        {
            std::shared_ptr<E> tmp;
            Switch<std::shared_ptr<E>>::Paste(tmp, node, id);
            p = tmp;
        }
    };

    /* handle raw binary-data through Srl::BitWrap */
    template<> struct Switch<BitWrap> {
        static const Type type = Type::Binary;

        static void Insert(const BitWrap& wrap, Node& node, const String& name)
        {
            if(wrap.data.size > 0 && wrap.data.ptr == nullptr) {
                auto msg = "Unable to insert value"
                    + (name.size() < 1 ? "" : " " + name.unwrap(false))
                    + ". Data pointer in BitWrap is null.";
                throw Exception(msg);
            }
            Value value(wrap.data, Type::Binary);
            node.insert_value(value, name);
        }

        template<class ID = String>
        static void Paste(BitWrap& wrap, const Value& value, const ID& id = Aux::str_empty)
        {
            Aux::check_type_string(value.type(), id);

            MemBlock mem_block(value.data(), value.size());

            auto size = value.type() == Type::String
                ? Tools::get_base64_decoded_size((const char*)mem_block.ptr, mem_block.size)
                : mem_block.size;

            if(!wrap.alloc) {
                Aux::throw_error("No alloc function set in BitWrap.", id);
            }
            uint8_t* data_dst = wrap.alloc(size);

            if(size == 0) {
                return;
            }

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

    template<class T> struct Switch<VecWrap<T>> {
        static const Type type = Switch<BitWrap>::type;

        static void Insert(const VecWrap<T>& wrap, Node& node, const String& name)
        {
            Switch<BitWrap>::Insert(wrap.bitwrap, node, name);
        }

        template<class ID = String>
        static void Paste(VecWrap<T>& wrap, const Value& value, const ID& id = Aux::str_empty)
        {
            Switch<BitWrap>::Paste(wrap.bitwrap, value, id);
        }
    };

    template<> struct Switch<Bytes> {

        static const Type type = Type::Binary;

        static void Insert(const Bytes& bytes, Node& node, const String& name)
        {
            BitWrap bit_wrap(bytes.buf.data(), bytes.buf.size());
            Switch<BitWrap>::Insert(bit_wrap, node, name);
        }

        template<class ID = String>
        static void Paste(Bytes& bytes, const Value& value, const ID& id = Aux::str_empty)
        {
            VecWrap v_wrap(bytes.buf);
            Switch<VecWrap<uint8_t>>::Paste(v_wrap, value, id);
        }
    };

    template<class V> struct Switch<HTable<std::string, V>> {
        static const Type type = Type::Object;

        static void Insert(const HTable<std::string, V>& ht, Node& node, const String& name)
        {
            node.open_scope(&Insert, type, name, ht);
        }

        static void Insert(Node& node, const HTable<std::string, V>& ht)
        {
            ht.foreach([&](const std::string& key, const V& value)
            {
                node.insert(key, value);
            });
        }

        template<class ID = String>
        static void Paste(HTable<std::string, V>& ht, Node& node, const ID& id = Aux::str_empty)
        {
            Aux::check_type(Type::Object, node.type(), id);

            ht.clear();

            if(node.parsed) {

                for(auto& itm : node.items<V>()) {
                    auto key = itm.field.name().unwrap();
                    ht.insert(std::move(key), itm.field.template unwrap<V>());

                }
                return;
            }

            while(true) {

                auto itm = node.consume_item<V>();
                if(node.parsed) {
                    return;
                }

                auto key = itm.name().unwrap();
                ht.insert(std::move(key), itm.template unwrap<V>());
                FinishScope(itm);
            }
        }

        template<class Item>
        static typename std::enable_if<std::is_same<Item, Value>::value, void>::type
        FinishScope(Item&) { }

        template<class Item>
        static typename std::enable_if<std::is_same<Item, Node>::value, void>::type
        FinishScope(Item& node) { node.consume_scope(); }
    };

    namespace Aux {
        /* auxiliary functions */
        template<class TChar, class ID>
        void copy_string(TChar* dst, size_t size, const Value& value, const ID& id)
        {
            check_type_string(value.type(), id);

            auto encoding = get_encoding<TChar>();

            if(value.encoding() == encoding) {
                check_size(size, value.size(), id);
                memcpy(dst, value.data(), size);

            } else {
                std::vector<uint8_t> buffer;
                String wrap(value.data(), value.size(), value.encoding());

                auto conv_size = Tools::conv_charset(encoding, wrap, buffer, true);

                check_size(size, conv_size, id);
                memcpy(dst, buffer.data(), size);
            }
        }

        inline void throw_error(const std::string& msg, size_t field_id)
        {
            auto message = "Unable to set field at pos : "  + std::to_string(field_id) + ". " + msg;
            throw Exception(message);
        }

        inline void throw_error(const std::string& msg, const String& field_id)
        {
            auto message = "Unable to set field" +
                (field_id.size() > 0 ? " " + field_id.unwrap(false) : "") + ". " + msg;
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
            if(got != Type::String && !TpTools::is_scalar(got)) {
                throw_error("Type mismatch, expected: string or literal, got: "
                            + TpTools::get_name(got) + ".", field_id);
            }
        }

        template<class ID>
        void check_type_string(Type got, const ID& field_id)
        {
            if(got != Type::String && got != Type::Binary) {
                throw_error("Type mismatch, expected: string or binary, got: "
                            + TpTools::get_name(got) + ".", field_id);
            }
        }

        template<class ID>
        void check_type_scope(Type got, const ID& field_id)
        {
            if(!TpTools::is_scope(got)) {
                throw_error("Type mismatch, expected: " + TpTools::get_name(Type::Object)
                            + " or " + TpTools::get_name(Type::Array) + ", got: "
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

        template<class ID>
        void check_size_max(size_t required_max_size, size_t actual_size, const ID& field_id)
        {
            if(actual_size > required_max_size) {
                throw_error("Size mismatch.", field_id);
            }
        }

        template<class T, class ID>
        typename std::enable_if<is_polymorphic<T*>::value, void>::type
        ptr_insert(T* const p, Node& node, const ID& id)
        {
            Switch<T*>::Insert(p, node, id);
        }

        template<class T, class ID>
        typename std::enable_if<!is_polymorphic<T*>::value, void>::type
        ptr_insert(T* const p, Node& node, const ID& id)
        {
            if(!p) {
                Switch<std::nullptr_t>::Insert(std::nullptr_t(), node, id);

            } else {
                Switch<T>::Insert(*p, node, id);
            }
        }
} } }

#endif
