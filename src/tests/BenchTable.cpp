#include "Tests.h"
#include "BasicStruct.h"

#include <chrono>
#include <fstream>
#include <memory>
#include <unistd.h>
#include <map>

using namespace std;
using namespace Srl;
using namespace Tests;

struct BStruct {

    BasicStruct strct;
    vector<string> str_vec;
    vector<double> fp_vec;
    vector<uint64_t> int_vec;
    BStruct() { }

    BStruct(string str)
    {
        for(int i = 0; i < 10; i++) {
            this->str_vec.push_back(str);
            this->fp_vec.push_back(i * 35.688f);
            this->int_vec.push_back(0xF099294CE1 << i);
        }
    }

    void srl_resolve(Context& ctx)
    {
        ctx ("struct", strct) ("vec", str_vec) ("fp_vec", VecWrap<double>(fp_vec)) ("int_vec", int_vec);
    }

};

void measure(const function<void(void)>& fnc, const string& msg,
             const function<void(void)>& rep = [] { },
             int fac = Tests::Benchmark_Iterations)
{
    auto start = chrono::high_resolution_clock::now();

    for(int i = 0; i < fac; i++) {
        fnc();
        rep();
    }

    auto end = chrono::high_resolution_clock::now();
    auto elapsed = (chrono::duration_cast<chrono::microseconds>(end - start).count() / 1000.0) / fac;

    print_log(msg + to_string(elapsed) + "\n");
}

void run_bench(Tree& tree)
{
    try {
        print_log("\nBenching data extraction...\n");

        map<string, BStruct> map;

        measure([&](){ tree.root().paste_field("map", map); }, "\tpaste data   ms: ");
        Tree new_tree;
        measure([&](){ new_tree.load_object(map); }, "\tinsert data  ms: ");

        vector<uint8_t> source;
        measure([&](){ source = Tree().store<PSrl>(map); }, "\tStore Srl    ms: ");

        measure([&](){ Tree().restore<PSrl>(map, source); }, "\tRestore Srl  ms: ");

        measure([&](){ source = Tree().store(map, PJson(true)); }, "\tStore Json   ms: ");

        measure([&](){ Tree().restore<PJson>(map, source); }, "\tRestore Json ms: ");

        measure([&](){ source = Tree().store<PMsgPack>(map); }, "\tStore MsgP    ms: ");

        measure([&](){ Tree().restore<PMsgPack>(map, source); }, "\tRestore MsgP  ms: ");

    } catch(Exception& ex) {
        print_log(string(ex.what()) + "\n");
    }
}

template<class T, class... Tail>
void run_bench(Tree& tree, T&& parser, const string& name, Tail&&... tail)
{
    try {
        print_log("\nBenching parser " + name +  "...\n");

        vector<uint8_t> source;
       

        Tree reuse;
        measure([&](){ tree.to_source(source, parser); },   "\tparse out  ms: ");
        measure([&](){ reuse.load_source(source, parser); }, "\tparse in   ms: ");

        measure([&](){ ofstream fs("File"); tree.to_source(fs, parser); },        "\twrite file ms: ", []{ }, 1);
        
        measure([&](){ ifstream fsi("File"); reuse.load_source(fsi, parser); },    "\tread file  ms: ", []{ }, 1);

        unlink("File");

        measure([&] {
            Lib::In src;
            src.set(source);
            int depth = 0;
            parser.clear();
            while(true) {
                auto seg = parser.read(src); 
                if(TpTools::is_scope(seg.second.type())) {
                    depth++;
                    continue;
                }
                if(seg.second.type() == Type::Scope_End) {
                    depth--;
                    if(depth <= 0) {
                        break;
                    }
                }
                
            }
        }, "\tParse in / no store ms: ");

        print_log("\tdocument size kbytes: " + to_string(source.size() / 1000.0f) + "\n");

    } catch (Exception& ex) {
        print_log(string(ex.what()) + "\n");
    }

    run_bench(tree, tail...);
}

void Tests::run_benches()
{
    Verbose = true;
    print_log("\nGenerating data with " + to_string(Benchmark_Objects) + " objects...");

    map<string, BStruct> data;

    measure([&](){
        for(auto i = 0U; i < Benchmark_Objects; i++) {
            data.insert({ to_string(i), BStruct(to_string(i + 1000)) });
        }
    }, "ms: ", [] { }, 1);

    Tree tree;
    tree.root().insert("map", data);

    print_log("Running with " + to_string(Benchmark_Iterations) + " iterations...\n");

    tree.to_source(PMsgPack());

    run_bench(
        tree,
        PSrl(), "PSrl",
        PMsgPack(), "PMsgPack",
        PJson(), "PJson",
        PJson(true), "PJson w/o space",
        PXml(true), "PXml w/o space"
    );

}
