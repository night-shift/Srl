#ifndef SRL_UTIL_H
#define SRL_UTIL_H

#include "Common.h"
#include "Out.h"
#include "In.h"

namespace Srl {

    template<class Object, class TParser>
    Object Restore(const std::vector<uint8_t>& source, const TParser& parser = TParser());

    template<class Object, class TParser>
    Object Restore(std::istream& stream, const TParser& parser = TParser());

    template<class Object, class TParser>
    Object Restore(const uint8_t* source, size_t size, const TParser& parser = TParser());

    template<class TParser, class Object>
    void Restore(Object& object, const std::vector<uint8_t>& source, const TParser& parser = TParser());

    template<class TParser, class Object>
    void Restore(Object& object, const uint8_t* source, size_t size, const TParser& parser = TParser());

    template<class TParser, class Object>
    void Restore(Object& object, std::istream& stream, const TParser& parser = TParser());

    template<class TParser, class Object>
    void Store(std::ostream& stream, const Object& object, const TParser& parser = TParser(), const std::string& name = "");

    template<class TParser, class Object>
    std::vector<uint8_t> Store(const Object& object, const TParser& parser = TParser(), const std::string& name = "");

    template<class TParser, class Object>
    void Store(const Object& object, Lib::Out& out, const TParser& parser, const std::string& name);
}

#endif
