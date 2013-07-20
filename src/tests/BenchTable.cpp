#include "Tests.h"
#include "BasicStruct.h"

#include <chrono>
#include <fstream>
#include <memory>
#include <unistd.h>

using namespace std;
using namespace Srl;
using namespace Tests;

struct BStruct {

    BasicStruct strct;
    vector<string> str_vec;

    BStruct() { }

    BStruct(string str)
    {
        for(int i = 0; i < 10; i++) {
            this->str_vec.push_back(str);
        }
    }

    void srl_resolve(Context& ctx)
    {
        ctx ("struct", strct) ("vec", str_vec);
    }
};

void measure(const function<void(void)>& fnc, const string& msg, int fac = 1)
{
    auto start = chrono::high_resolution_clock::now();
    fnc();
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start).count() / fac;
    print_log(msg + to_string(elapsed) + "\n");
}

void run_bench(Tree& tree)
{
    try {
        print_log("\nBenching data extraction...\n");

        map<string, BStruct> map;

        measure([&](){ tree.root().paste_field("map", map); }, "\tpaste data   ms: ");
        Tree new_tree;
        measure([&](){ new_tree = Tree::From_Type(map); }, "\tinsert data  ms: ");

        vector<uint8_t> source;
        measure([&](){ source = Store<PSrl>(map); }, "\tStore Srl    ms: ");

        measure([&](){ Restore<PSrl>(map, source); }, "\tRestore Srl  ms: ");

        measure([&](){ source = Store(map, PJson(true)); }, "\tStore Json   ms: ");

        measure([&](){ Restore<PJson>(map, source); }, "\tRestore Json ms: ");

        measure([&](){ source = Store<PMsgPack>(map); }, "\tStore MsgP    ms: ");

        measure([&](){ Restore<PMsgPack>(map, source); }, "\tRestore MsgP  ms: ");

    } catch(Exception& ex) {
        print_log(string(ex.what()) + "\n");
    }
}

template<class T, class... Tail>
void run_bench(Tree& tree, const T& parser, const string& name, const string& message, const Tail&... tail)
{
    try {
        print_log("\nBenching parser " + name + message + "...\n");

        vector<uint8_t> source;
        measure([&](){ source = tree.to_source(parser); },   "\tparse out  ms: ");
        measure([&](){ Tree::From_Source(source, parser); }, "\tparse in   ms: ");

        ofstream fs("File");
        measure([&](){ tree.to_source(fs, parser); },        "\twrite file ms: ");
        fs.close();

        ifstream fsi("File");
        measure([&](){ Tree::From_Source(fsi, parser); },    "\tread file  ms: ");
        fsi.close();

        unlink("File");

        measure([&] {
            Lib::In src(source.data(), source.size());
            auto p = parser;
            int depth = 0;
            while(true) {
                auto seg = p.parse_in(src); 
                if(TpTools::is_scope(seg.value.type())) {
                    depth++;
                    continue;
                }
                if(seg.value.type() == Type::Scope_End) {
                    depth--;
                    if(depth <= 0) {
                        break;
                    }
                }
                
            }
        }, "\tParse in / no store ms: ");

        print_log("\tdocument size bytes: " + to_string(source.size()) + "\n");

    } catch (Exception& ex) {
        print_log(string(ex.what()) + "\n");
    }

    run_bench(tree, tail...);
}

void Tests::run_benches()
{
    Verbose = true;
    print_log("\nGenerating data with " + to_string(Benchmark_Objects) + " objects...");

    map<string, BStruct> map;

    measure([&](){
        for(auto i = 0U; i < Benchmark_Objects; i++) {
            map.insert({ to_string(i), BStruct(to_string(i + 1000)) });
        }
    }, "ms: ");

    Tree tree;
    tree.root().insert("map", map);
    run_bench (
        tree,
        PSrl(),  "Srl",  "",
        PMsgPack(),  "MsgPack",  "",
        PJson(), "Json", "",
        PXml(),  "Xml",  "",
        PJson(true), "Json"," w/o space",
        PXml(true),  "Xml", " w/o space"
    );
}
