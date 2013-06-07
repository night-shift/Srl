#ifndef SRL_UTIL_H
#define SRL_UTIL_H

#include "Common.h"
#include "Out.h"
#include "In.h"

namespace Srl {

    namespace Lib { namespace Aux {

        template<class TParser, typename Data>
        void Store(Data& data, Out& out, const TParser& parser, const std::string& name);

        template<class TParser, typename Data>
        void Restore(Data& data, In& source, const TParser& parser);
    } }

    template<typename Data, class TParser>
    void Restore(Data& data, const uint8_t* source, size_t size, const TParser& parser = TParser());

    template<class Data, class TParser>
    Data Restore(const std::vector<uint8_t>& source, const TParser& parser = TParser());

    template<class Data, class TParser>
    Data Restore(std::istream& stream, const TParser& parser = TParser());

    template<class Data, class TParser>
    Data Restore(const uint8_t* source, size_t size, const TParser& parser = TParser());

    template<class TParser, class Data>
    void Restore(Data& data, const std::vector<uint8_t>& source, const TParser& parser = TParser());

    template<class TParser, typename Data>
    void Store(std::ostream& stream, Data& data, const TParser& parser = TParser(), const std::string& name = "");

    template<class TParser, typename Data>
    std::vector<uint8_t> Store(Data& data, const TParser& parser = TParser(), const std::string& name = "");
}

#endif
