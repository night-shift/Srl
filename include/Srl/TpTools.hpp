#ifndef SRL_TYPETOOLS_HPP
#define SRL_TYPETOOLS_HPP

#include "Type.h"
#include "Cast.hpp"

#include <limits>

namespace Srl { namespace TpTools {

    const size_t Literal_Max_Size = 8;

    template<class T> struct SrlType;
    template<Type name>  struct Real;
    template<Type name>  struct TypeSize { static const size_t size = 0; };

    #define SRL_TYPE_TEMPLATES(name, value, real, size_) \
        template<> struct SrlType<real>        { static const Type type = Type::name; }; \
        template<> struct Real<Type::name>     { typedef real type;                   }; \
        template<> struct TypeSize<Type::name> { static const size_t size = size_;    };

    SRL_TYPES_LITERAL(SRL_TYPE_TEMPLATES)

    #undef SRL_TYPE_TEMPLATES

    namespace Aux {

        #define SRL_TYPE_SIZE_LOOKUP_ENTRY(name, value, type, size) size,

        const size_t type_size_lookup[] {
            SRL_TYPES_LITERAL(SRL_TYPE_SIZE_LOOKUP_ENTRY)
            SRL_TYPES_NON_SCALAR(SRL_TYPE_SIZE_LOOKUP_ENTRY)
        };
        #undef SRL_TYPE_SIZE_LOOKUP_ENTRY

        #define SRL_TYPE_NAME_LOOKUP_ENTRY(name, value, type, size) #name,

        const std::string type_name_lookup[] {
            SRL_TYPES_LITERAL(SRL_TYPE_NAME_LOOKUP_ENTRY)
            SRL_TYPES_NON_SCALAR(SRL_TYPE_NAME_LOOKUP_ENTRY)
        };
        #undef SRL_TYPE_NAME_LOOKUP_ENTRY
    }

    template<class T>
    bool paste_type(T& target, Type src_type, const uint8_t* src_mem);

    inline const std::string& get_name (Type type) { return Aux::type_name_lookup[(size_t)type]; }

    constexpr bool is_scope    (Type type)     { return type == Type::Object || type == Type::Array; }
    constexpr bool is_literal  (Type type)     { return type < Type::String; }
    constexpr bool is_integral (Type type)     { return type < Type::FP32; }
    constexpr bool is_fp       (Type type)     { return type == Type::FP32 || type == Type::FP64; }
    constexpr bool is_signed   (Type type)     { return is_fp(type) || (is_integral(type) && (uint8_t)type % 2 == 1); }
    constexpr bool is_valid    (uint8_t value) { return value < sizeof(Aux::type_size_lookup); }
    inline size_t  get_size    (Type type)     { return Aux::type_size_lookup[(size_t)type]; }

    namespace Aux {
        constexpr bool gt(size_t a, size_t b) { return a > b; }
    }

    template<class T, class U>
    typename std::enable_if<Aux::gt(sizeof(U), sizeof(T)) && !std::numeric_limits<U>::is_signed, bool>::type
    overflows(const U& val)
    {
        return val > std::numeric_limits<T>::max();
    }

    template<class T, class U>
    typename std::enable_if<Aux::gt(sizeof(U), sizeof(T)) && std::numeric_limits<U>::is_signed, bool>::type
    overflows(const U& val)
    {
        return val > std::numeric_limits<T>::max() || val < std::numeric_limits<T>::min();
    }

    template<class T, class U>
    typename std::enable_if<Aux::gt(sizeof(T), sizeof(U)) , bool>::type
    overflows(const U&)
    {
        return false;
    }

    template<class T, class U>
    typename std::enable_if<sizeof(T) == sizeof(U) && !std::is_floating_point<T>::value, bool>::type
    overflows(const U& val)
    {
        return !std::numeric_limits<U>::is_signed && std::numeric_limits<T>::is_signed
                    ? val > std::numeric_limits<U>::max() >> 1
                    : false;
    }

    template<class T, class U>
    typename std::enable_if<sizeof(T) == sizeof(U) && std::is_floating_point<T>::value, bool>::type
    overflows(const U&)
    {
        return false;
    }

    template<class T, Type TP>
    typename std::enable_if<is_integral(TP), bool>::type
    paste_type_switch(T& target, const uint8_t* mem)
    {
        auto val = Lib::Read_Cast<typename Real<TP>::type>(mem);

        if(is_signed(TP) && !std::numeric_limits<T>::is_signed
           && val >> (TypeSize<TP>::size - 1) * 8 & 0x80) {
             /*trying to assign a negative value to an unsigned type */
            return false;
        }

        if(overflows<T>(val)) {
            return false;
        }

        target = val;
        return true;
    }

    template<class T, Type TP>
    typename std::enable_if<is_fp(TP), bool>::type
    paste_type_switch(T& target, const uint8_t* mem)
    {
        auto val = Lib::Read_Cast<typename Real<TP>::type>(mem);

        if(val < std::numeric_limits<T>::min() || val > std::numeric_limits<T>::max()) {
            return false;
        }

        target = val;
        return true;
    }

    template<class T, Type TP>
    typename std::enable_if<TP == Type::Null, bool>::type
    paste_type_switch(T& target, const uint8_t*)
    {
        target = T();
        return true;
    }

    #define SRL_PASTE_TYPE_TEMPLATE(type, value, real, size) \
        case Type::type : return paste_type_switch<T, Type::type>(target, src_mem);

    template<class T>
    bool paste_type(T& target, Type src_type, const uint8_t* src_mem)
    {
        static_assert(is_literal(SrlType<T>::type), "Cannot paste non-literal type.");

        switch(src_type) {
            SRL_TYPES_LITERAL(SRL_PASTE_TYPE_TEMPLATE)

            default : assert("Incomplete literal-type entries."); return false;
        }
    }

    #undef SRL_PASTE_TYPE_TEMPLATE

} }


#endif
