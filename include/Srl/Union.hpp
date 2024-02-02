#ifndef SRL_UNION_HPP
#define SRL_UNION_HPP

#include "Srl/String.h"
#include "Srl/TpTools.h"
#include "Union.h"
#include "Node.hpp"
#include "Value.hpp"
#include <exception>
#include <optional>
#include <string>

namespace Srl {

    template<class T>
    inline void Union::throw_conversion_error(std::exception& prev_ex)
    {
        check_not_empty();

        Type        union_type;
        std::string union_name;

        if(this->is_value()) {
            union_name = valptr->name().unwrap(false);
            union_type = valptr->type();
        }

        if(this->is_node()) {
            union_name = nodeptr->name().unwrap(false);
            union_type = nodeptr->type();
        }

        auto target_type = Lib::Switch<T>::type;

        auto message = "Failed to convert union <" + union_name + "> of type "
                       "<" + TpTools::get_name(union_type) + "> to type "
                       "<" + TpTools::get_name(target_type) +"> because the following error occured: " +
                       std::string(prev_ex.what());

        throw Exception(message);
    }

    inline void Union::check_not_empty()
    {
        if(!nodeptr && !valptr) {
            throw Exception("Srl::Union is empty.");
        }
    }

    inline void Union::check_node()
    {
        Union::check_not_empty();

        if(!this->is_node()) {
            throw Exception("Union <" + valptr->name().unwrap(false) + ">" + " cannot be converted to requested type");
        }
    }

    inline void Union::check_value()
    {
        Union::check_not_empty();

        if(!this->is_value()) {
            throw Exception("Union <" + nodeptr->name().unwrap(false) + ">" + " cannot be converted to requested type");
        }
    }

    inline Union Union::get(const String& name)
    {
        check_not_empty();

        if(!nodeptr) {
            throw Exception("Cannot access field <" + name.unwrap(false) + "> in "
                            "union <" + valptr->name().unwrap(false) + "> because union is of type "
                            "<" + TpTools::get_name(valptr->type()) + ">");
        }

        try {

            return this->nodeptr->get(name);

        } catch(...) {

            throw Exception("Union <" + nodeptr->name().unwrap(false) + ">" + " has no field <" + name.unwrap(false) + ">");
        }

        return Union();
    }

    inline std::optional<Union> Union::try_get(const String& name)
    {
        if(!this->is_node()) {
            return { };
        }
        return this->nodeptr->try_get(name);
    }

    template<class T>
    Union::operator T()
    {
        return this->unwrap<T>();
    }

    template<class T>
    typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), T>::type
    Union::unwrap()
    {
        try {

            check_value();
            return this->valptr->unwrap<T>();

        } catch(std::exception& ex) {

            this->throw_conversion_error<T>(ex);
        }

        return Union();
    }

    template<class T>
    typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), T>::type
    Union::unwrap()
    {
        try {

            check_node();
            return this->nodeptr->unwrap<T>();

        } catch(std::exception& ex) {

            this->throw_conversion_error<T>(ex);
        }

        return Union();
    }

    template<class T>
    typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), void>::type
    Union::copy_to(T& v)
    {
        try {

            check_value();
            this->value()->paste(v);

        } catch(std::exception& ex) {

            this->throw_conversion_error<T>(ex);
        }
    }

    template<class T>
    typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), void>::type
    Union::copy_to(T& n)
    {
        check_node();
        this->node()->paste(n);
    }

    template<class T>
    std::optional<typename std::enable_if<!TpTools::is_scope(Lib::Switch<T>::type), T>::type>
    Union::unwrap_if()
    {
        if(this->is_value()) {

            try {
                T res = this->valptr->unwrap<T>();
                return { std::move(res) };

            } catch(...) { }
        }

        return { };
    }

    template<class T>
    std::optional<typename std::enable_if<TpTools::is_scope(Lib::Switch<T>::type), T>::type>
    Union::unwrap_if()
    {
        if(this->is_node()) {

            try {
                T res = this->nodeptr->unwrap<T>();
                return { std::move(res) };

            } catch(...) { }
        }

        return { };
    }

}

#endif
