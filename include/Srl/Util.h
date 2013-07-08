#ifndef SRL_UTIL_H
#define SRL_UTIL_H

#include "Common.h"
#include "Out.h"
#include "In.h"

namespace Srl {

    namespace Lib { namespace Aux {

        template<class TParser, typename Object>
        void Store(const Object& object, Out& out, const TParser& parser, const std::string& name);

        template<class TParser, typename Object>
        void Restore(Object& object, In& source, const TParser& parser);
    } }

    template<typename Object, class TParser>
    void Restore(Object& object, const uint8_t* source, size_t size, const TParser& parser = TParser());

    template<class Object, class TParser>
    Object Restore(const std::vector<uint8_t>& source, const TParser& parser = TParser());

    template<class Object, class TParser>
    Object Restore(std::istream& stream, const TParser& parser = TParser());

    template<class Object, class TParser>
    Object Restore(const uint8_t* source, size_t size, const TParser& parser = TParser());

    template<class TParser, class Object>
    void Restore(Object& object, const uint8_t* source, size_t size, const TParser& parser = TParser());

    template<class TParser, class Object>
    void Restore(Object& object, const std::vector<uint8_t>& source, const TParser& parser = TParser());

    template<class TParser, class Object>
    void Restore(Object& object, std::istream& stream, const TParser& parser = TParser());

    template<class TParser, typename Object>
    void Store(std::ostream& stream, const Object& object, const TParser& parser = TParser(), const std::string& name = "");

    template<class TParser, typename Object>
    std::vector<uint8_t> Store(const Object& object, const TParser& parser = TParser(), const std::string& name = "");
}

#endif
