#ifndef SRL_RESOLVE_H
#define SRL_RESOLVE_H

#include "Common.h"
#include "Enums.h"
#include "Type.h"

#include <memory>

namespace Srl {

    /* Restored types are created with this template. As default parameterless
     * constructors are required. You can make Ctor a friend class
     * if you don't want to expose public default constructors.
     * Or make your own type-specific routines by defining a specialized
     * namespace Srl { template<>struct Srl::Ctor<your_namespace::your_type>...
     * template in your project. */
    template<class T, class... Rest> struct Ctor {

        static T                  Create()     { return T(); }
                                                 /* TODO replace with std::make_unique */
        static std::unique_ptr<T> Create_New() { return std::unique_ptr<T>(new T()); }
    };

    class Context;
    class StoreContext;
    class RestoreContext;
    class String;
    class Node;
    struct TypeID;

namespace Lib {

    struct MemBlock;

    template<class TString> std::pair<Encoding, MemBlock> wrap_string(const TString& str);

    template<class TChar> Encoding get_encoding();

    template<class T> struct has_store_method {

        template<class U, U> struct test_sig { };

        template <class U>
        static char test(test_sig<void(U::*)(Srl::StoreContext&) const, &U::srl_store>*);
        template <class U>
        static long test(...);

        static const bool value = sizeof(test<T>(0)) == sizeof(char);
    };

    template<class T> struct has_restore_method {

        template<class U, U> struct test_sig { };

        template <class U>
        static char test(test_sig<void(U::*)(Srl::RestoreContext&), &U::srl_restore>*);
        template <class U>
        static long test(...);

        static const bool value = sizeof(test<T>(0)) == sizeof(char);
    };

    template<class T> struct has_resolve_method {

        template<class U, U> struct test_sig { };

        template <class U>
        static char test(test_sig<void(U::*)(Srl::Context&), &U::srl_resolve>*);
        template <class U>
        static long test(...);

        static const bool value = sizeof(test<T>(0)) == sizeof(char);
    };

    template<class T> struct has_type_id_method {

        template<class U, U> struct test_sig { };

        template<class U>
        static char test(test_sig<const Srl::TypeID&(U::*)(void), &U::srl_type_id>*);
        template<class U>
        static long test(...);

        static const bool value = sizeof(test<T>(0)) == sizeof(char);
    };

    /* Switch<T> resolves types. Basically it tells Srl::Node how to
     * store and restore types.
     * 'Batteries included'-specializations are implemented in Resolve.hpp.
     * Types that can't be resolved end up in this default template. */
    template<class T, class = void> struct Switch {
        static const Type type = Type::Null;

        static_assert(has_resolve_method<T>::value,
        "Srl error. Unable to resolve type. Did you implement either a srl_resolve(Context&) "
        "or both srl_store(StoreContext&) and srl_restore(RestoreContext&) methods?"
        );
    };

    template<class T> struct is_polymorphic {
        static const bool value = std::is_pointer<T>::value &&
                                  has_resolve_method<typename std::remove_pointer<T>::type>::value &&
                                  has_type_id_method<typename std::remove_pointer<T>::type>::value;
    };

    /* type-trait templates */
    template<class T> struct has_iterator {
        template <class U> static char test(typename U::const_iterator*);
        template <class U> static long test(...);
        static const bool value = sizeof(test<T>(0)) == sizeof(char);
    };

    template<class T> struct is_unique_ptr {
        static const bool value = false;
    };

    template<class T> struct is_unique_ptr<std::unique_ptr<T>> {
        static const bool value = true;
    };

    template<class T> struct is_weak_ptr {
        static const bool value = false;
    };

    template<class T> struct is_weak_ptr<std::weak_ptr<T>> {
        static const bool value = true;
    };

    template<class T> struct is_shared_ptr {
        static const bool value = false;
    };

    template<class T> struct is_shared_ptr<std::shared_ptr<T>> {
        static const bool value = true;
    };

    template<class T> struct is_basic_string {
        static const bool value = false;
    };

    template<class T> struct is_basic_string<std::basic_string<T>> {
        static const bool value = true;
    };

    template<class T, typename = void> struct is_container {
        static const bool value = false;
    };

    template<class T>
    struct is_container<T, typename std::enable_if<has_iterator<T>::value && !is_basic_string<T>::value>::type> {

        typedef typename T::const_iterator I;

        template<class U> static char
        begin(typename std::enable_if<std::is_same<decltype(static_cast<I (U::*)() const>(&U::begin)),
                                                   I(U::*)() const>::value, void
                                                  >::type*);
        template<class U> static long begin(...);

        template<class U> static char
        end(typename std::enable_if<std::is_same<decltype(static_cast<I (U::*)() const>(&U::end)),
                                                  I(U::*)() const>::value, void
                                                >::type*);
        template<class U> static long end(...);

        static const bool value = sizeof(end<T>(0))   == sizeof(char) &&
                                  sizeof(begin<T>(0)) == sizeof(char);
    };

    template<class T> struct is_char {
        typedef typename std::remove_const<T>::type TNC;
        static const bool value =
            std::is_same<char, TNC>::value     || std::is_same<wchar_t, TNC>::value ||
            std::is_same<char16_t, TNC>::value || std::is_same<char32_t, TNC>::value;
    };

    template<class T> struct array_type {
        typedef typename std::remove_pointer<typename std::decay<T>::type>::type type;
    };

    template<class T> struct is_char_array {
        static const bool value =
            std::is_array<T>::value &&
            is_char<typename array_type<T>::type>::value;
    };

    template<class T> struct is_cstr_pointer {
        static const bool value =
            std::is_pointer<T>::value &&
            is_char<typename std::remove_pointer<T>::type>::value;
    };

    template<class T> struct is_numeric {
        static const bool value =
            !is_char<T>::value &&
            (std::is_floating_point<T>::value || std::is_integral<T>::value);
    };

} }


#endif
