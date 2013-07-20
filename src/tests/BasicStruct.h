#ifndef BASIC_STRUCT_H
#define BASIC_STRUCT_H

#include "Tests.h"

namespace Tests {

    #define TEST(assertion) \
        if(!(assertion)) { throw Srl::Exception(std::string("Assertion [") + #assertion +"] in " + SCOPE + " failed.");  }

    #define FIELDS(X, T, P) \
        X(a##T, T, P) X(b##T, T, P) X(c##T, T, P) \
        X(d##T, T, P) X(e##T, T, P) X(f##T, T, P)

    #define FIELDS_FNC(X, P) \
        FIELDS(X, char,     P) FIELDS(X, double,  P) FIELDS(X, float,    P) \
        FIELDS(X, uint8_t,  P) FIELDS(X, int8_t,  P) FIELDS(X, int16_t,  P) \
        FIELDS(X, uint32_t, P) FIELDS(X, int32_t, P) FIELDS(X, uint64_t, P)

    #define DECLARE_FIELD(N, T, P) \
        T N = P;

    #define RESOLVE_FIELD(N, T, P) \
        (#N, N)

    #define COMPARE_FIELD(N, T, P) \
        TEST(N == P.N)

    #define SHUFFLE_FIELD(N, T, P) \
         N = P;

    #define INSERT_FIELD(N, T, P) \
        tree.root().insert(#N, N);

    #define PASTE_FIELD(N, T, P) \
        tree.root().paste_field(#N, N);

    struct BasicStruct {

        std::string SCOPE = "BasicStruct";

        FIELDS_FNC(DECLARE_FIELD, 120)

        void srl_resolve(Srl::Context& ctx)
        {
            ctx
            FIELDS_FNC(RESOLVE_FIELD, 0)
            ;
        }

        void test(BasicStruct& n)
        {
            FIELDS_FNC(COMPARE_FIELD, n)
        }

        void shuffle()
        {
            FIELDS_FNC(SHUFFLE_FIELD, 70)
        }

        void insert(Srl::Tree& tree)
        {
            FIELDS_FNC(INSERT_FIELD, 0);
        }

        void paste(Srl::Tree& tree)
        {
            FIELDS_FNC(PASTE_FIELD, 0);
        }

    };
}

namespace Srl {
    template<> struct Ctor<Tests::BasicStruct> {
        static Tests::BasicStruct Create() { return Tests::BasicStruct(); }
    };
}

#endif
