#include "Srl/Hash.h"
#include "Srl/Tree.h"
#include "Tests.h"
#include "BasicStruct.h"

#include <list>
#include <forward_list>
#include <queue>
#include <cmath>
#include <deque>
#include <stack>
#include <set>
#include <unordered_map>
#include <map>
#include <limits>
#include <fstream>
#include <sstream>
#include <unistd.h>

using namespace Srl;
using namespace std;
using namespace Tests;

struct TestClassG {

    const string SCOPE = "TestClassG";

    double d_max = 0.0;
    double d_min = 0.0;
    double d_low = 0.0;

    float f_max = 0.0;
    float f_min = 0.0;
    float f_low = 0.0;

    void srl_resolve (Context& ctx)
    {
        ctx ("d_max", d_max)
            ("d_min", d_min)
            ("d_low", d_low)
            ("f_low", f_low)
            ("f_max", f_max)
            ("f_min", f_min);
    }

    void shuffle()
    {
        d_max = numeric_limits<double>::max();
        d_min = numeric_limits<double>::min();
        d_low = numeric_limits<double>::lowest();

        f_max = numeric_limits<float>::max();
        f_min = numeric_limits<float>::min();
        f_low = numeric_limits<float>::lowest();
    }

    void test(TestClassG& n)
    {
        TEST(d_min == n.d_min)
        TEST(d_low == n.d_low)
        TEST(d_max == n.d_max)

        TEST(f_min == n.f_min)
        TEST(f_max == n.f_max)
        TEST(f_low == n.f_low)
    }
};

struct TestClassF  {

    const string SCOPE = "TestClassF";

    string some_string = "string";
    int integer_0 = 0;
    int integer_1 = 1;
    int integer_2 = 2;
    vector<char32_t> vec_char { U'5' };
    vector<BasicStruct> vec_struct { };
    TestClassG class_g;

    void srl_resolve (Context& ctx)
    {
        ctx(vec_struct)(integer_2)(some_string)
           (vec_char)(integer_0)(integer_1)
           (class_g);
    }

    void shuffle()
    {
        this->integer_0 = this->integer_1;
        this->integer_1 = this->integer_2;
        this->integer_2 = -1;
        this->vec_char.push_back( U'u' );
        this->vec_char.push_back( U'A' );
        this->vec_struct.push_back(BasicStruct());
        this->class_g.shuffle();
    }

    void test(TestClassF& n)
    {
        TEST(this->integer_0 == n.integer_0)
        TEST(this->integer_1 == n.integer_1)
        TEST(this->integer_2 == n.integer_2)
        TEST(this->vec_char  == n.vec_char);

        TEST(this->vec_struct.size() == n.vec_struct.size())
        for(auto i = 0U; i < this->vec_struct.size(); i++) {
            this->vec_struct[i].test(n.vec_struct[i]);
        }
        this->class_g.test(n.class_g);
    }
};

struct TestClassE {

    void srl_resolve (Context& ctx) { double d; ctx(d); }
};

struct TestClassD {

    const string SCOPE = "TestClassD";

    TestClassE class_e;
    TestClassF class_f;

    list<int> list_integer { 4 };
    deque<string> deque_string { "a" };
    map<string, TestClassE> map_class;
    vector<TestClassE> vector_class { TestClassE() };

    void srl_resolve (Context& ctx)
    {
        ctx ("list_integer", list_integer)
            ("deque_string", deque_string)
            ("map_class", map_class)
            ("class_e", class_e)
            ("vector_class", vector_class)
            ("class_f", class_f);
    }

    void shuffle()
    {
        this->list_integer = { 0, 1, 2147483647, -1 };
        this->deque_string.clear();

        for(auto& e : this->list_integer) {
            this->deque_string.push_back(to_string(e));
            this->vector_class.push_back(TestClassE());
        }

        this->map_class.insert({ "a", TestClassE()});
        this->class_f.shuffle();
    }

    void test(TestClassD& n)
    {
        TEST(this->list_integer.size() == n.list_integer.size())
        TEST(this->list_integer == n.list_integer)

        TEST(this->deque_string.size() == n.deque_string.size())
        TEST(this->deque_string == n.deque_string)

        TEST(this->map_class.size() == n.map_class.size());
        TEST(this->vector_class.size() == n.vector_class.size());

        this->class_f.test(n.class_f);
    }

};

struct TestClassC {

    const string SCOPE = "TestClassC";

    unordered_map<string, TestClassD> map_class {
        { "first", TestClassD() }
    };

    vector<vector<int>> vector_nested {
        vector<int> { 2 }
    };

    Srl::Lib::HTable<string, int>               hash_table_a;
    Srl::Lib::HTable<string, TestClassD>        hash_table_b;
    Srl::Lib::HTable<string, vector<int>>       hash_table_c;

    set<string> set_string { "a" };

    void srl_restore(RestoreContext& ctx)
    {
        ctx ("map_class", map_class)
            ("vector_nested", vector_nested)
            ("set_string", set_string)
            ("hash_table_a", hash_table_a)
            ("hash_table_b", hash_table_b)
            ("hash_table_c", hash_table_c);
    }

    void srl_store(StoreContext& ctx) const
    {
        ctx ("map_class", map_class)
            ("vector_nested", vector_nested)
            ("set_string", set_string)
            ("hash_table_a", hash_table_a)
            ("hash_table_b", hash_table_b)
            ("hash_table_c", hash_table_c);
    }

    void shuffle()
    {
        this->hash_table_a.clear();
        this->hash_table_b.clear();
        this->hash_table_c.clear();
        this->map_class.clear();

        for(auto i = 0; i < 3; i++) {

            auto vec = vector<int>{ 3 + i, 6 + i, 8 + i};

            auto map_key = to_string(i);

            this->map_class.insert({map_key, TestClassD()});
            this->vector_nested.push_back(vec);
            this->set_string.insert(map_key);

            this->hash_table_a.insert(map_key, i);
            this->hash_table_b.insert(map_key, TestClassD());
            this->hash_table_c.insert(map_key, vec);
        }

        for(auto& e : this->map_class) {
            e.second.shuffle();
        }

        hash_table_b.foreach([&](const string& _, TestClassD& v)
        {
            v.shuffle();
        });
    }

    void test(TestClassC& n)
    {
        TEST(this->map_class.size() == n.map_class.size())
        for(auto& e : this->map_class) {
            TEST(n.map_class.find(e.first) != n.map_class.end())
            e.second.test(n.map_class[e.first]);
        }

        TEST(this->vector_nested.size() == n.vector_nested.size())
        for(auto i = 0U; i < this->vector_nested.size(); i++) {
            TEST(this->vector_nested[i] == n.vector_nested[i]);
        }

        TEST(this->set_string == n.set_string);

        TEST(this->hash_table_a.num_entries() == n.hash_table_a.num_entries());
        TEST(this->hash_table_b.num_entries() == n.hash_table_b.num_entries());
        TEST(this->hash_table_c.num_entries() == n.hash_table_c.num_entries());

        this->hash_table_a.foreach([&](const string& key, int& value)
        {
            auto* n_entry = n.hash_table_a.get(key);
            TEST(n_entry != nullptr);
            TEST(*n_entry == value);
        });

        this->hash_table_b.foreach([&](const string& key, TestClassD& value)
        {
            auto* n_entry = n.hash_table_b.get(key);
            TEST(n_entry != nullptr);
            n_entry->test(value);
        });

        this->hash_table_c.foreach([&](const string& key, vector<int>& value)
        {
            auto* n_entry = n.hash_table_c.get(key);
            TEST(n_entry != nullptr);
            TEST(n_entry->size() == value.size());
            for(auto i = 0U; i < n_entry->size(); i++) {
                TEST(n_entry->at(i) == value.at(i));
            }
        });
    }
};

class TestClassB {

    const string SCOPE = "TestClassB";

public :
    const size_t binary_sz = 2000000;

    void srl_resolve (Context& ctx)
    {
        auto wrap = BitWrap(raw_binary, 10, [this](size_t size) {
            TEST(size == 10)
            return this->raw_binary;
        });

        auto wrap_med = BitWrap(bin_med, sizeof(bin_med), [this](size_t size) {
            TEST(size == sizeof(bin_med))
            return this->bin_med;
        });

        auto wrap_large = BitWrap(bin_large, sizeof(bin_large), [this](size_t size) {
            TEST(size == sizeof(bin_large))
            return this->bin_large;
        });

        ctx ("huge_binary", VecWrap<uint8_t>(huge_binary))
            ("string", string_)
            (u"string_u16", string_u16)
            (U"string_u32", string_u32)
            ("char_u16", char_u16)
            ("char_u32", char_u32)
            ("char_w", char_w)
            ("raw_binary", wrap)
            ("bin_med", wrap_med)
            ("bin_large", wrap_large)
            ("str_med", str_med)
            ("str_large", str_large)
            ("str_xlarge", str_xlarge);
    }

    void shuffle()
    {
        string_ = u8"\u6c34\U0001d10b";
        string_u32 = U"\U0001D122\U0001D122";
        string_u16 = u"\xD834\xDD60";
        char_u16 = u'\x266A';
        char_u32 = U'\U0001D122';
        char_w = L'\x266A';

        for(auto& e : raw_binary) {
            e <<= 3;
        }

        int strl = sizeof(str_med) - 1;

        for(int i = 0; i < strl; i++) {
            str_med[i] = 'b';
        }

        strl = sizeof(str_large) - 1;
        for(int i = 0; i < strl; i++) {
            str_large[i] = 'b';
        }

        strl = sizeof(str_xlarge) - 1;
        for(int i = 0; i < strl; i++) {
            str_xlarge[i] = 'b';
        }

        int binl = sizeof(bin_med);

        for(int i = 0; i < binl; i++) {
            bin_med[i] = 1;
        }

        binl = sizeof(bin_large);
        for(int i = 0; i < binl; i++) {
            bin_large[i] = 1;
        }

        huge_binary.resize(binary_sz);

        for(auto i = 0U; i < binary_sz ; i ++) {
            huge_binary[i] = i % 128;
        }

    }

    void test(TestClassB& n)
    {
        TEST(string_ == n.string_)
        TEST(string_u32 == n.string_u32)
        TEST(string_u16 == n.string_u16)
        TEST(char_u16 == n.char_u16)
        TEST(char_u32 == n.char_u32)
        TEST(char_w == n.char_w)
        TEST(memcmp(raw_binary, n.raw_binary, 10) == 0);
        TEST(huge_binary == n.huge_binary);

        TEST(strcmp(str_med, n.str_med) == 0);
        TEST(strcmp(str_large, n.str_large) == 0);
        TEST(strcmp(str_xlarge, n.str_xlarge) == 0);

        TEST(memcmp(bin_med, n.bin_med, sizeof(bin_med)) == 0);
        TEST(memcmp(bin_large, n.bin_large, sizeof(bin_large)) == 0);
    }

private :

    string string_        = "string";
    u16string string_u16  = u"string";
    u32string string_u32  = U"string";

    char str_med[255] { 'a' };
    char str_large[513] { 'a' };
    char str_xlarge[100000] { 'a' };

    uint8_t bin_med[255] { 0 };
    uint8_t bin_large[513] { 0 };


    char16_t char_u16 = u'c';
    char32_t char_u32 = U'c';
    wchar_t  char_w   = L'c';

    uint8_t raw_binary[10] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    };

    vector<uint8_t> huge_binary;
};

struct TestClassA {

    const string SCOPE = "TestClassA";


    bool     bool_  = false;
    int64_t s_int64_n = 1;
    int64_t  s_int64 = 1;
    int16_t  s_int16 = 1;
    uint64_t u_int64 = 1;
    long double long_double = 2.2;
    unsigned char u_char    = 'a';
    const char*   c_string  = "c_str";

    int array[5] = {
        3, 6, 9, 12, 15
    };

    BasicStruct basic_struct;

    unique_ptr<TestClassB> nested_class_b = unique_ptr<TestClassB>(new TestClassB());
    shared_ptr<TestClassC> nested_class_c = shared_ptr<TestClassC>(new TestClassC());
    tuple<int, vector<double>, string> tpl { 0, vector<double> { 0.0 }, "s" };

    void srl_resolve (Context& ctx)
    {
        ctx ("array", array)
            ("basic_struct", basic_struct)
            ("nested_class_b", nested_class_b)
            ("nested_class_c", nested_class_c)
            ("long_double", long_double)
            ("bool", bool_) ("u_char", u_char)
            ("s_int64_n", s_int64_n)
            ("s_int64", s_int64)
            ("s_int16", s_int16)
            ("u_int64", u_int64)
            ("tpl", tpl)
            ("c_string", c_string);
    }

    void shuffle()
    {
        basic_struct.shuffle();
        nested_class_b->shuffle();
        nested_class_c->shuffle();
        long_double *= 2.139;
        u_char *= 3;
        bool_ = !bool_;

        s_int64_n = -1;
        s_int64 = numeric_limits<int64_t>::max();
        s_int16 = numeric_limits<int16_t>::min();
        u_int64 = numeric_limits<uint64_t>::max();

        for(auto& e : array) {
            e *= -3;
        }

        get<0>(tpl) = 10;
        get<1>(tpl) = { 1.0, 2.0 };
        get<2>(tpl) = "str";
    }

    void test(TestClassA& n)
    {
        basic_struct.test(n.basic_struct);

        for(auto i = 0; i < 5; i++) {
            TEST(array[i] == n.array[i]);
        }

        auto epsilon = 0.00000000001;
        TEST(fabs(long_double - n.long_double) < epsilon)

        TEST(u_char == n.u_char)
        TEST(bool_ == n.bool_)
        TEST(s_int64_n == n.s_int64_n);
        TEST(s_int64 == n.s_int64);
        TEST(s_int16 == n.s_int16);
        TEST(u_int64 == n.u_int64);
        TEST(c_string == n.c_string);

        nested_class_b->test(*n.nested_class_b);
        nested_class_c->test(*n.nested_class_c);

        TEST(tpl == n.tpl);
    }
};

bool test_serialize() { return true; }

template<class TParser, class... Tail>
bool test_serialize(TParser&& parser, const string& parser_name, Tail&&... tail)
{
    bool success = true;

    try {
        Tests::print_log("\nTest serialize " + parser_name + "\n");
        TestClassA original;
        original.shuffle();

        print_log("\tSrl::Store...............");
        auto source = Tree().store(original, parser);
        print_log("ok.\n");

        print_log("\tSrl::Restore.............");
        auto restored = Tree().restore<TestClassA>(source, parser);
        print_log("ok.\n");

        print_log("\tData comparison..........");
        restored.test(original);
        print_log("ok.\n");

        print_log("\tTree::load_object........");
        Tree tree;
        tree.load_object(original);
        print_log("ok.\n");

        print_log("\tTree::to_source..........");
        source = tree.to_source(parser);
        print_log("ok.\n");

        print_log("\tTree::load_source........");
        tree.load_source(source, parser);
        print_log("ok.\n");

        print_log("\tTree::root().paste.......");
        TestClassA target;
        tree.root().paste(target);

        print_log("ok.\n");

        print_log("\tData comparison..........");
        target.test(original);
        print_log("ok.\n");

        print_log("\tTree::to_stream..........");
        stringstream strm;
        tree.load_object(original);
        tree.to_source(strm, parser);
        print_log("ok.\n");

        strm.seekg(0);

        print_log("\tTree::from_stream........");
        tree.load_source(strm, parser);
        tree.root().paste(target);
        print_log("ok.\n");

        print_log("\tData comparison..........");
        target.test(original);
        print_log("ok.\n");

        print_log("\tSrl::Restore unordered...");
        tree.load_object(original);
        source = tree.to_source(parser);
        auto restored_unordered = Tree().restore<TestClassA>(source, parser);
        print_log("ok.\n");

        print_log("\tData comparison..........");
        restored_unordered.test(original);
        print_log("ok.\n");

        print_log("\tTree::pack...............");
        vector<uint8_t> buf;
        original.shuffle();
        Srl::Tree().pack(buf, parser, "data", original);
        print_log("ok.\n");

        print_log("\tTree::unpack.............");
        TestClassA unpack_target;
        Srl::Tree().unpack(buf, parser, "data", unpack_target);
        print_log("ok.\n");

        print_log("\tData comparison..........");
        unpack_target.test(original);
        print_log("ok.\n");

    }  catch(Srl::Exception& ex) {
        Tests::print_log(string(ex.what()) + "\n");
        success = false;
    }

    return test_serialize(tail...) && success;
}

bool Tests::test_parser()
{
    bool success = test_serialize (
        PSrl(),  "Srl",  PMsgPack(), "MsgPack",
        PJson(), "Json",
        PJson(false), "Json w/ space"
    );

    return success;
}

