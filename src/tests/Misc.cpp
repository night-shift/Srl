#include "Tests.h"
#include "BasicStruct.h"
#include <list>
#include <memory>

using namespace std;
using namespace Srl;
using namespace Tests;

bool test_node_api()
{
    string SCOPE = "Api test";

    print_log("\tNode api...");

    try {
        Tree tree;
        auto& root = tree.root();

        root.insert("field0", 0, "field1", 1, "field2", 2);
        root.insert("node0", root);
        root.insert("node1", root.node("node0"));
        root.node("node0").insert("node1", root.node("node0"));
        root.node("node0").insert("field3", 3);

        TEST(root.get("node0").get("field3").unwrap<int>() == 3);

        bool recursive  = true;
        size_t n_nodes  = 3;
        size_t n_values = (n_nodes + 1) * 3 + 1;

        TEST(root.has_node("node0"));
        TEST(root.has_node("node1"));
        TEST(!root.has_node("node2"));

        TEST(root.has_value("field0"));
        TEST(root.has_value("field1"));
        TEST(root.has_value("field2"));
        TEST(!root.has_value("field3"));

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
        root.foreach_node([&counted_nodes](Node&) { counted_nodes++; }, recursive);
        TEST(counted_nodes == n_nodes)

        size_t counted_values = 0;
        root.foreach_value([&counted_values](Value&) { counted_values++; }, recursive);
        TEST(counted_values == n_values)

        auto before = root.num_nodes();
        root.remove_node("node0");
        TEST(root.num_nodes() == (before - 1))

        before = root.num_values();
        root.remove_value("field0");
        TEST(root.num_values() == (before - 1))

        root.foreach_node([](Node& node) {
            auto values = node.all_values();
            for(auto* v : values) {
                node.remove_value(v->name());
            }
        }, recursive);

        counted_values = 0;
        root.foreach_value([&counted_values](...) { counted_values++; }, recursive);

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
        tree.root().insert("str", str);

        print_log("\tString escaping json...");
        tree.load_source(tree.to_source(PJson()), PJson());

        TEST(str == tree.root().unwrap_field<string>("str"));
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
        tree.root().insert(
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

        auto source = tree.to_source(PJson());
        tree.load_source(tree.to_source(PJson()), PJson());
        auto& node = tree.root();

        auto& value = node.value("int");
        auto some_int = value.unwrap<int>();
        TEST(some_int == 5)

        value = node.value("string");
        auto some_string = value.unwrap<string>();
        TEST(some_string == "string")

        node.node("nested_node").paste_field("string", some_string);
        TEST(some_string == "string");

        auto nodes = node.find_nodes("nested_node", true);
        TEST(nodes.size() == 2)

        auto vec = nodes.front()->unwrap_field<vector<int>>("vector");
        vector<int> vec_n { 5, 7, 8 };
        TEST(vec == vec_n)

        auto lst = node.node("nested_node").unwrap_field<list<list<string>>>("list");
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


struct Base {

    virtual int get() = 0;
    virtual const Srl::TypeID& srl_type_id() = 0;
    virtual void srl_resolve(Context& ctx) = 0;

    virtual ~Base() { }
};

struct Root : Base {

    int m = 0;
    int get() override { return m; }

    const Srl::TypeID& srl_type_id() override;

    virtual void srl_resolve(Context& ctx) override
    {
        ctx ("root_field", m);
    }

    virtual ~Root() { }
};

const auto reg_root = Srl::register_type<Root>("Root");
const Srl::TypeID& Root::srl_type_id() { return reg_root; }

struct DerivedA : Root {

    DerivedA(int i = 1) : a(i) { }

    int a;
    int get() override { return a; }

    const Srl::TypeID& srl_type_id() override;

    void srl_resolve(Context& ctx) override
    {
        Root::srl_resolve(ctx);
        ctx ("a_field", a);
    }
};

const auto reg_a = Srl::register_type<DerivedA>("DerivedA");
const Srl::TypeID& DerivedA::srl_type_id() { return reg_a; }

class DerivedB : public Root {

    friend struct Srl::Ctor<DerivedB>;

public:
    DerivedB(int i) : b(i) { }

    int get() override { return b; }

    const Srl::TypeID& srl_type_id() override;

    void srl_resolve(Context& ctx) override
    {
        Root::srl_resolve(ctx);
        ctx ("b_field", b);
    }

private:
    int b;
    DerivedB() :b(0) { }
};

const auto reg_b = Srl::register_type<DerivedB>("DerivedB");
const Srl::TypeID& DerivedB::srl_type_id() { return reg_b; }

struct TestClass {
    unique_ptr<Base> one;
    unique_ptr<Base> two;

    TestClass(Base* one_ = nullptr, Base* two_ = nullptr) : one(one_), two(two_) { }

    void srl_resolve(Context& ctx)
    {
        ctx ("one", one) ("two", two);
    }
};

bool test_polymorphic_classes()
{
    const string SCOPE = "Serializing polymorphic classes";

    print_log("\t" + SCOPE + "...");
    try {

        TestClass cl(new DerivedB(12), new DerivedA(6));
        cl = Tree().restore<TestClass, PJson>(Tree().store<PJson>(cl));

        TEST(strcmp(cl.one->srl_type_id().name(), "DerivedB") == 0);
        TEST(strcmp(cl.two->srl_type_id().name(), "DerivedA") == 0);
        TEST(cl.one->get() == 12);
        TEST(cl.two->get() == 6);

    } catch(Exception& ex) {
        print_log(string(ex.what()) + "\n");
        return false;
    }

    print_log("ok.\n");
    return true;
}


struct Shared {

    shared_ptr<Base> base0;
    weak_ptr<Base>   base1;

    void srl_resolve(Context& ctx)
    {
        ctx("base0", base0) ("base1", base1);
    }
};

bool test_shared_references()
{
    const string SCOPE = "Serializing shared references";
    print_log("\t" + SCOPE + "...");

    shared_ptr<Base> res0;
    shared_ptr<Base> res1;
    weak_ptr<Base>   res2;

    try {

        {
            auto s0 = shared_ptr<Base>(new DerivedA());
            weak_ptr<Base> s1 = s0;

            Shared shared { s0, s1 };

            auto bytes = Tree().store<PJson>(shared);
            Tree tree;
            tree.load_source(bytes, PJson());

            res0 = tree.root().node("base0").unwrap<shared_ptr<Base>>();
            res1 = tree.root().node("base1").unwrap<shared_ptr<Base>>();
            res2 = tree.root().node("base1").unwrap<weak_ptr<Base>>();
        }

        TEST(res0.get() == res1.get())
        TEST(res0.get() == res2.lock().get())

    } catch(Exception& ex) {

        print_log(string(ex.what()) + "\n");
        return false;
    }

    print_log("ok.\n");
    return true;
}

bool test_pointer_serializing()
{
    const string SCOPE = "Serializing pointers";
    print_log("\t" + SCOPE + "...");

    struct PtrTestStruct {
        int               a = 0;
        int               b = 1;
        int*              c = nullptr;
        int*              d = nullptr;
        Root*             e = nullptr;

        vector<TestClass*> vec;

        void srl_resolve(Srl::Context& ctx)
        {
            ctx ("a", a)("b", b)("vec", vec)("c", c)
                ("d", d) ("e", e);
        }

        ~PtrTestStruct()
        {
        }
    };

    vector<shared_ptr<PtrTestStruct>> storage;
    vector<shared_ptr<int>>           int_storage;
    vector<shared_ptr<Root>>          obj_storage;
    vector<shared_ptr<TestClass>>     tst_storage;
    vector<PtrTestStruct*>            pointers;

    for(int i = 0; i < 10; i++) {
        auto ptr = make_shared<PtrTestStruct>();
        ptr->a = i;
        ptr->b = i * 3;

        auto int_ptr = make_shared<int>();
        ptr->c = int_ptr.get();
        *ptr->c = i * 6;

        for(int j = i; j < 10; j++) {

            auto cl_ptr = make_shared<TestClass>(new DerivedB(j), new DerivedA(j * i + 2));
            tst_storage.push_back(cl_ptr);
            ptr->vec.push_back(cl_ptr.get());
        }

        auto root_ptr = make_shared<Root>();
        root_ptr->m = i * 7;
        ptr->e = root_ptr.get();

        storage.push_back(ptr);
        int_storage.push_back(int_ptr);
        obj_storage.push_back(root_ptr);
        pointers.push_back(ptr.get());
    }

    vector<PtrTestStruct> restored;

    try {
        vector<uint8_t> bytes;
        Srl::Tree().store(pointers, bytes, Srl::PSrl());
        Srl::Tree().restore(restored, bytes, Srl::PSrl());


    } catch(Exception& ex) {

        print_log(string(ex.what()) + "\n");
        return false;
    }

    TEST(restored.size() == pointers.size());

    for(auto i = 0U; i < pointers.size(); i++) {

        auto& a = restored[i];
        auto& b = *pointers[i];

        TEST(a.a == b.a);
        TEST(a.b == b.b);
        TEST(a.c && b.c && *a.c == *b.c);
        TEST(a.d == nullptr && a.d == b.d);
        TEST(a.e && b.e && a.e->m == b.e->m);

        TEST(a.vec.size() == b.vec.size());


        for(auto j = 0U; j < a.vec.size(); j++) {
            auto& a_sub = *a.vec[j];
            auto& b_sub = *b.vec[j];

            TEST(a_sub.one->srl_type_id().name() == b_sub.one->srl_type_id().name());
            TEST(a_sub.two->srl_type_id().name() == b_sub.two->srl_type_id().name());

            TEST(a_sub.one->get() == b_sub.one->get());
            TEST(a_sub.two->get() == b_sub.two->get());
        }
    }

    print_log("ok.\n");
    return true;
}

bool Tests::test_misc()
{
    print_log("\nTest misc\n");

    bool success = test_document_building();
    success &= test_string_escaping();
    success &= test_node_api();
    success &= test_polymorphic_classes();
    success &= test_shared_references();
    success &= test_pointer_serializing();

    return success;
}
