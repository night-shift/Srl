#ifndef SRL_TYPETOOLS_H
#define SRL_TYPETOOLS_H

#include "Type.h"
#include "Value.h"

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

    SRL_TYPES_SCALAR(SRL_TYPE_TEMPLATES)

    #undef SRL_TYPE_TEMPLATES

    namespace Aux {

        #define SRL_TYPE_SIZE_LOOKUP_ENTRY(name, value, type, size) size,

        const size_t type_size_lookup[] {
            SRL_TYPES_SCALAR(SRL_TYPE_SIZE_LOOKUP_ENTRY)
            SRL_TYPES_NON_SCALAR(SRL_TYPE_SIZE_LOOKUP_ENTRY)
        };
        #undef SRL_TYPE_SIZE_LOOKUP_ENTRY

        #define SRL_TYPE_NAME_LOOKUP_ENTRY(name, value, type, size) #name,

        const std::string type_name_lookup[] {
            SRL_TYPES_SCALAR(SRL_TYPE_NAME_LOOKUP_ENTRY)
            SRL_TYPES_NON_SCALAR(SRL_TYPE_NAME_LOOKUP_ENTRY)
        };
        #undef SRL_TYPE_NAME_LOOKUP_ENTRY
    }

    template<class T>
    bool apply_type(T& target, Type src_type, const uint8_t* src_mem);

    inline const std::string& get_name (Type type) { return Aux::type_name_lookup[(size_t)type]; }

    constexpr bool is_scope    (Type type)     { return type == Type::Object || type == Type::Array; }
    constexpr bool is_scalar   (Type type)     { return type < Type::String; }
    constexpr bool is_num      (Type type)     { return type < Type::Null && type > Type::Bool; }
    constexpr bool is_integral (Type type)     { return type < Type::FP32; }
    constexpr bool is_fp       (Type type)     { return type == Type::FP32 || type == Type::FP64; }
    constexpr bool is_signed   (Type type)     { return is_fp(type) || (is_integral(type) && (uint8_t)type % 2 == 1); }
    constexpr bool is_valid    (uint8_t value) { return value < sizeof(Aux::type_size_lookup); }
    inline size_t  get_size    (Type type)     { return Aux::type_size_lookup[(size_t)type]; }

    namespace Aux {

        constexpr bool gt(size_t a, size_t b) { return a > b; }

        template<class T>
        constexpr T max() { return std::numeric_limits<T>::max(); }

        template<class T>
        constexpr T min() { return std::numeric_limits<T>::lowest(); }

        template<class T, class U>
        typename std::enable_if<Aux::gt(sizeof(U), sizeof(T)) && !std::is_signed<U>::value, bool>::type
        overflows(const U& val)
        {
            return val > max<T>();
        }

        template<class T, class U>
        typename std::enable_if<Aux::gt(sizeof(U), sizeof(T)) && std::is_signed<U>::value, bool>::type
        overflows(const U& val)
        {
            return val > max<T>() || val < min<T>();
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
            return !std::is_signed<U>::value && std::is_signed<T>::value
                        ? val > max<U>() >> 1
                        : false;
        }

        template<class T, class U>
        typename std::enable_if<sizeof(T) == sizeof(U) && std::is_floating_point<T>::value, bool>::type
        overflows(const U&)
        {
            return false;
        }

        template<class T, Type TP>
        typename std::enable_if<is_integral(TP) && is_signed(TP), bool>::type
        try_apply(T& target, const Value& value)
        {
            int64_t val = value.pblock().i64;

                /*trying to assign a negative value to an unsigned type */
            if((!std::is_signed<T>::value && val < 0) || overflows<T>(val)) {
                return false;
            }

            target = val;

            return true;
        }

        template<class T, Type TP>
        typename std::enable_if<is_integral(TP) && !is_signed(TP) && TP != Type::Bool, bool>::type
        try_apply(T& target, const Value& value)
        {
            uint64_t val = value.pblock().ui64;

            if(overflows<T>(val)) {
                return false;
            }

            target = val;
            return true;
        }

        template<class T, Type TP>
        typename std::enable_if<TP == Type::Bool, bool>::type
        try_apply(T& target, const Value& value)
        {
            auto val = value.pblock().i64;

            target = val;
            return true;
        }

        template<class T, Type TP>
        typename std::enable_if<is_fp(TP), bool>::type
        try_apply(T& target, const Value& value)
        {
            double val = TP == Type::FP32 ? value.pblock().fp32 : value.pblock().fp64;

            if(SrlType<T>::type != TP && SrlType<T>::type != Type::FP64 &&
              (val < min<T>() || val > (double)max<T>())) {

                return false;
            }

            target = TP == Type::FP32 ? value.pblock().fp32 : value.pblock().fp64;
            return true;
        }

        template<class T, Type TP>
        typename std::enable_if<TP == Type::Null, bool>::type
        try_apply(T& target, const Value&)
        {
            target = T();
            return true;
        }
    }

    #define SRL_APPLY_TEMPLATE(type, idx, real, size) \
    case Type::type : return Aux::try_apply<T, Type::type>(target, value);

    template<class T>
    bool apply_type(T& target, const Value& value)
    {
        static_assert(is_scalar(SrlType<T>::type), "Cannot paste non-literal type.");

        switch(value.type()) {
            SRL_TYPES_SCALAR(SRL_APPLY_TEMPLATE)

            default : assert(false && "Incomplete literal-type entries."); return false;
        }
    }

    #undef SRL_APPLY_TEMPLATE

} }


#endif
