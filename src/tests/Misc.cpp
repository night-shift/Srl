#include "Tests.h"
#include "BasicStruct.h"
#include <list>

using namespace std;
using namespace Srl;
using namespace Tests;

bool test_node_api()
{
    string SCOPE = "Api test";

    print_log("\tNode api...");

    try {
        Tree tree;
        auto& root = *tree.root();

        root.insert("field0", 0, "field1", 1, "field2", 2);
        root.insert("node0", root);
        root.insert("node1", *root.node("node0"));
        root.node("node0")->insert("node1", *root.node("node0"));
        root.node("node0")->insert("field3", 3);

        bool recursive  = true;
        size_t n_nodes  = 3;
        size_t n_values = (n_nodes + 1) * 3 + 1;

        auto all_nodes = root.all_nodes(recursive);
        TEST(all_nodes.size() == n_nodes)

        auto all_values = root.all_values(recursive);
        TEST(all_values.size() == n_values)

        auto nodes_w_name = root.find_nodes("node1", recursive);
        TEST(nodes_w_name.size() == 2)

        auto values_w_name = root.find_values("field0", recursive);
        TEST(values_w_name.size() == (n_nodes + 1))

        values_w_name = root.find_values("field3", recursive);
        TEST(values_w_name.size() == 1)

        size_t counted_nodes = 0;
        root.forall_nodes([&counted_nodes](...) { counted_nodes++; }, recursive);
        TEST(counted_nodes == n_nodes)

        size_t counted_values = 0;
        root.forall_values([&counted_values](...) { counted_values++; }, recursive);
        TEST(counted_values == n_values)

        auto before = root.num_nodes();
        root.remove_node("node0");
        TEST(root.num_nodes() == (before - 1))

        before = root.num_values();
        root.remove_value("field0");
        TEST(root.num_values() == (before - 1))

        root.forall_nodes([](Node* node) {
            auto values = node->all_values();
            for(auto v : values) {
                node->remove_value(*v->name());
            }
        }, recursive);

        counted_values = 0;
        root.forall_values([&counted_values](...) { counted_values++; }, recursive);

        TEST(counted_values == root.num_values())


    } catch(Srl::Exception& ex) {
        print_log(string(ex.what()) + "\n");
        return false;
    }

    print_log("ok.\n");

    return true;
}

bool test_string_escaping()
{
    string SCOPE = "String escaping";

    try {
        string str = "\"quotes\" & ampersand gr >  ls < ap ' \n\b\t\r\f/ backslash \\";

        Tree tree;
        tree.root()->insert("str", str);

        print_log("\tString escaping xml...");
        tree = Tree::From_Source<PXml>(tree.to_source<PXml>());
        TEST(str == tree.root()->unwrap_field<string>("str"));
        print_log("ok.\n");

        print_log("\tString escaping json...");
        tree = Tree::From_Source<PJson>(tree.to_source<PJson>());

        TEST(str == tree.root()->unwrap_field<string>("str"));
        print_log("ok.\n");

    } catch(Srl::Exception& ex) {
        print_log(string(ex.what()) + "\n");
        return false;
    }

    return true;
}

bool test_document_building()
{
    string SCOPE = "Document building";

    try {
        print_log("\tDocument building...");

        Tree tree;
        tree.root()->insert(
            "string", "string",
            "int", 5,
            "nested_node", Node(tree).insert(
                "string", U"string",
                "vector", vector<int> {
                    5, 7, 8
                },
                "list", list<list<string>> {
                    { "a", "b", "c" },
                    { "d", "e", "f" }
                },
                "nested_node", Node(tree).insert(
                    "int", 10
                )
            ),
            "double", 10.0
        );

        auto source = tree.to_source<PJson>();
        tree = Tree::From_Source<PJson>(tree.to_source<PJson>());
        auto& node = *tree.root();

        auto value = node.value("int");
        auto some_int = value->unwrap<int>();
        TEST(some_int == 5)

        value = node.value("string");
        auto some_string = value->unwrap<string>();
        TEST(some_string == "string")

        node.node("nested_node")->paste_field("string", some_string);
        TEST(some_string == "string");

        auto nodes = node.find_nodes("nested_node", true);
        TEST(nodes.size() == 2)

        auto vec = nodes[0]->unwrap_field<vector<int>>("vector");
        vector<int> vec_n { 5, 7, 8 };
        TEST(vec == vec_n)

        some_int = nodes[1]->unwrap_field<int>("int");
        TEST(some_int == 10)

        auto lst = node.node("nested_node")->unwrap_field<list<list<string>>>("list");
        list<list<string>> lst_n { list<string> { "a", "b", "c" }, list<string> { "d", "e", "f" } };
        TEST(lst == lst_n)

        double some_double;
        node.paste_field("double", some_double);
        TEST(some_double == 10.0)

        Tree g_basic;
        BasicStruct s_basic;
        s_basic.shuffle();
        s_basic.insert(g_basic);

        BasicStruct r_basic;
        r_basic.paste(g_basic);
        s_basic.test(r_basic);

        print_log("ok.\n");

        return true;

    } catch(Srl::Exception& ex) {
        print_log(string(ex.what()) + "\n");
        return false;
    }
}

bool test_xml_attributes()
{
    const string SCOPE = "Xml attribute parsing";

    try {

        print_log("\tXml attribute extraction...");

        string xml = "<node attribute1 = \"   12  \" attribute2=\"value\" attribute3  =  \"  -10\"/>";

        Tree tree = Tree::From_Source<PXml>((const uint8_t*)xml.c_str(), xml.size());

        double attribute1 = 0;
        string attribute2 = "";
        int attribute3    = 0;

        tree.root()->paste_field("attribute1", attribute1);
        tree.root()->paste_field("attribute2", attribute2);
        tree.root()->paste_field("attribute3", attribute3);

        TEST(*tree.root()->name() == "node");
        TEST(attribute1 == 12.0)
        TEST(attribute2 == "value")
        TEST(attribute3 == -10)

        print_log("ok.\n");

        return true;

    } catch (Srl::Exception& ex) {
        print_log(string(ex.what()) + "\n");
        return false;
    }
}

bool Tests::test_misc()
{
    print_log("\nTest misc\n");

    bool success = test_xml_attributes();
    success &= test_document_building();
    success &= test_string_escaping();
    success &= test_node_api();

    return success;
}
