#ifndef SRL_STORAGE_HPP
#define SRL_STORAGE_HPP

#include "Storage.h"

namespace Srl { namespace Lib {

    inline std::list<Node*>& Storage::nodes()
    {
        return this->stored_nodes;
    }

    inline std::vector<uint8_t>& Storage::str_conv_buffer()
    {
        return this->str_buffer;
    }

    inline std::vector<uint8_t>& Storage::type_conv_buffer()
    {
        return this->type_buffer;
    }

} }

#endif
