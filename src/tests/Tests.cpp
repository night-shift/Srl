#include "Tests.h"
#include <sstream>

using namespace std;
using namespace Tests;

bool Tests::Verbose = false;
bool Tests::Run_Benchmarks = false;
size_t Tests::Benchmark_Objects = 20000;

void Tests::print_source(const vector<uint8_t>& source)
{
    for(auto c : source) {
        cout << (const char)c;
    }
    cout<<endl;
}

void Tests::print_log(const std::string& message)
{
    if(Tests::Verbose) {
        cout<<message;
    }
}

int main(int argc, char** args)
{
    for(int i = 0; i < argc; i++) {
        if(string(args[i]) == "-v") {
            Tests::Verbose = true;
        } else if(string(args[i]) == "-b") {
            Tests::Run_Benchmarks = true;
        } else if(string(args[i], 3) == "-i=") {
            istringstream is(string(args[i] + 3));
            int in;
            is >> in;
            if(in > 0) { Tests::Benchmark_Objects = in; }
        }
    }

      if(Tests::Run_Benchmarks) {
        Tests::run_benches();

    } else {
        bool success = Tests::test_parser();
        success &= Tests::test_malicious_input();
        success &= Tests::test_misc();

        cout<<endl;
        if(success) {
            cout<<"Tests passed."<<endl;
        } else {
            cout<<"Tests failed."<<endl;
        }
    }

    return 0;
}
