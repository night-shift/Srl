#include "Srl/PJson.h"
#include "Srl/Tree.h"
#include "Tests.h"
#include "BasicStruct.h"

using namespace std;
using namespace Srl;
using namespace Tests;

struct TestStruct {

    vector<BasicStruct> vec;

    TestStruct()
    {
        for(int i = 0; i < 10; i++) {
            vec.push_back(BasicStruct());
        }
    }

    void srl_resolve(Context& ctx)
    {
        ctx ("vec", vec);
    }
};

bool malicious_input()
{
    print_log("\nTest malicious input Tree\n");

    bool success = false;
    TestStruct m;
    Tree tree;
    tree.load_object(m);
    auto& node = tree.root();

    try {
        print_log("\tOut of bounds access...");
        node.node(2);
        print_log("failed.\n");

    } catch(Exception& ex) {
        success = true;
        print_log("ok. -> " + string(ex.what()) + "\n");
    }

    try {
        print_log("\tWrong field access.....");
        node.node("cev");
        print_log("failed.\n");

    } catch(Exception& ex) {
        success &= true;
        print_log("ok. -> " + string(ex.what()) + "\n");
    }

    try {
        print_log("\tType mismatch..........");
        node.get("vec").unwrap<int>();
        success = false;
        print_log("failed.\n");

    } catch(Exception& ex) {
        success &= true;
        print_log("ok. -> " + string(ex.what()) + "\n");
    }

    try {
        print_log("\tOverflow int string....");
        node.insert("string", to_string(numeric_limits<uint64_t>::max()) + "1");
        node.unwrap_field<uint64_t>("string");
        success = false;
        print_log("failed.\n");

    } catch(Exception& ex) {
        success &= true;
        print_log("ok. -> " + string(ex.what()) + "\n");
    }

    try {
        print_log("\tOverflow integer max...");
        auto i = (uint64_t)(numeric_limits<int64_t>::max()) + 1;
        node.insert("uint", i);
        node.unwrap_field<int64_t>("uint");
        success = false;
        print_log("failed.\n");

    } catch(Exception& ex) {
        success &= true;
        print_log("ok. -> " + string(ex.what()) + "\n");
    }

    try {
        print_log("\tOverflow integer min...");
        int64_t i = numeric_limits<int64_t>::min();
        node.insert("int", i);
        node.unwrap_field<int32_t>("int");
        success = false;
        print_log("failed.\n");

    } catch(Exception& ex) {
        success &= true;
        print_log("ok. -> " + string(ex.what()) + "\n");
    }

    try {
        print_log("\tOverflow fp max........");
        auto d = numeric_limits<double>::max();
        node.insert("double_max", d);
        node.unwrap_field<float>("double_max");
        success = false;
        print_log("failed.\n");

    } catch(Exception& ex) {
        success &= true;
        print_log("ok. -> " + string(ex.what()) + "\n");
    }

    try {
        print_log("\tOverflow fp min........");
        auto d = numeric_limits<double>::lowest();
        node.insert("double_min", d);
        node.unwrap_field<float>("double_min");
        success = false;
        print_log("failed.\n");

    } catch(Exception& ex) {
        success &= true;
        print_log("ok. -> " + string(ex.what()) + "\n");
    }

    try {
        print_log("\tOverflow fp inf to integer........");
        auto str = "123123e100000";
        node.insert("inf_fp_str", str);

        node.get("inf_fp_str").unwrap<uint64_t>();
        success = false;
        print_log("failed.\n");

    } catch(Exception& ex) {
        success &= true;
        print_log("ok. -> " + string(ex.what()) + "\n");
    }

    try {
        print_log("\tNegative to unsigned...");
        short s = -1;
        node.insert("short", s);
        node.unwrap_field<unsigned short>("short");
        success = false;
        print_log("failed.\n");

    } catch(Exception& ex) {
        success &= true;
        print_log("ok. -> " + string(ex.what()) + "\n");
    }

    string source_nested = "";
    for(auto i = 0U; i < 1024 * 10; i++) {
        source_nested += "{\"n\":{";
    }

    try {

        print_log("\tExcessive nesting with '{' load source...");

        Srl::Tree tree_ld;
        tree_ld.load_source(source_nested, Srl::PJson());
        success = false;
        print_log("failed.\n");

    } catch(Exception& ex) {
        success &= true;
        print_log("ok. -> " + string(ex.what()) + "\n");
    }

    source_nested = "";
    for(auto i = 0U; i < 1024 * 10; i++) {
        source_nested += "[";
    }

    try {

        print_log("\tExcessive nesting with '[' unpack...");

        TestStruct ts;
        Srl::Tree().unpack(source_nested, Srl::PJson(), "data", ts);
        success = false;
        print_log("failed.\n");

    } catch(Exception& ex) {
        success &= true;
        print_log("ok. -> " + string(ex.what()) + "\n");
    }

    return success;
}

template<class TParser, class... Tail>
bool malicious_input(TParser&& parser, const string& parser_name, Tail&&... tail)
{
    print_log("\nTest malicious input " + parser_name + "\n");

    bool success = false;
    TestStruct m;
    auto data = Tree().store(m, parser);

    assert(data.size() > 100);
    /* wild guess */
    for(auto i = 10U; i < data.size(); i += 11) {
        data[i] ^= ~0U;
    }

    try {
        if(Verbose) cout<<"\tParsing...";
        Tree().load_source(data, parser);
        print_log("failed.\n");

    } catch(Exception& ex) {
        print_log("ok. -> " + string(ex.what()) + "\n");
        success = true;
    }
    return malicious_input(tail...) && success;
}


bool Tests::test_malicious_input()
{
    bool success = malicious_input (
        PSrl(),  "Srl", PMsgPack(), "MsgPack",
        PJson(), "Json",
        PJson(false), "Json w/ space"
    );

    return success;
}


