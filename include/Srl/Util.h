#ifndef SRL_UTIL_H
#define SRL_UTIL_H

#include "Common.h"
#include "Out.h"
#include "In.h"

namespace Srl {

    template<class Object, class TParser>
    Object Restore(const std::vector<uint8_t>& source, TParser&& parser = TParser());

    template<class Object, class TParser>
    Object Restore(std::istream& stream, TParser&& parser = TParser());

    template<class Object, class TParser>
    Object Restore(const uint8_t* source, size_t size, TParser&& parser = TParser());

    template<class TParser, class Object>
    void Restore(Object& object, const std::vector<uint8_t>& source, TParser&& parser = TParser());

    template<class TParser, class Object>
    void Restore(Object& object, const uint8_t* source, size_t size, TParser&& parser = TParser());

    template<class TParser, class Object>
    void Restore(Object& object, std::istream& stream, TParser&& parser = TParser());

    template<class TParser, class Object>
    std::vector<uint8_t> Store(const Object& object, TParser&& parser = TParser());

}
#endif
