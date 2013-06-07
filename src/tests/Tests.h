#ifndef TESTS_H
#define TESTS_H

#include "Srl/Srl.h"

#include <iostream>

namespace Tests {

    #define TEST(assertion) \
        if(!(assertion)) { throw Srl::Exception(std::string("Assertion [") + #assertion +"] in " + SCOPE + " failed.");  }

    extern bool   Verbose;
    extern bool   Run_Benchmarks;
    extern size_t Benchmark_Objects;

    void print_source(const std::vector<uint8_t>& source);

    void print_log(const std::string& message);

    bool test_parser();

    bool test_misc();

    bool test_malicious_input();

    void run_benches();
}

#endif
