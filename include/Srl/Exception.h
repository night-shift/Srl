#ifndef SRL_EXCEPTION_H
#define SRL_EXCEPTION_H

#include "Common.h"

namespace Srl {

    class Exception : public std::exception {

    public:
        Exception(const std::string& message_)
            : message("Srl error: " + message_) { }

        ~Exception() throw() { }

        const char* what() const throw() { return this->message.c_str(); }

    private:
        std::string message;

    };
}

#endif
