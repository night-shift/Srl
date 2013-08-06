## Srl
```cpp
#include "Srl/Srl.h"

class YourClass {
    // handles basic data types
    int a = 10;
    // strings
    u32string b = U"string";
    // stl-containers...
    vector<double> c { 1.0, 2.0, 3.0 };
    // ...including maps
    map<short, string> d { { 0, "0" }, { 1, "1" } };
    // shared_ / unique_ptrs
    shared_ptr<int> e = make_shared<int>(5);
    // tuples
    tuple<bool, string, char> f { false, "tuple", 'N' };

public :
    // implement a srl_resolve method
    void srl_resolve (Srl::Context& ctx) {
        // tell the context what to serialize, field names are optional
        ctx ("a", a) ("b", b) ("c", c)
            ("d", d) ("e", e) ("f", f);
    }
};

int main() {
    YourClass original;
    // serialize with Srl::Store
    vector<uint8_t> bytes = Srl::Store<Srl::PJson>(original);
    //                                     |-> choose a parser
    // deserialize with Srl::Restore
    YourClass restored = Srl::Restore<YourClass, Srl::PJson>(bytes);
    // or use streams
    ofstream fso("file");
    Srl::Store<Srl::PMsgPack>(fso, original);
    ifstream fsi("file");
    Srl::Restore<Srl::PMsgPack>(restored, fsi);
    // Thats it.
    return 0;
}
```
#### Processing documents
```cpp
string json = "{ "
    "\"name\"    : \"cpp\","
    "\"version\" : 11"
"}";
// create Srl::Tree from a source
Tree tree = Tree::From_Source<PJson>(json.c_str(), json.size());
// the root node
Node& root = tree.root();
// access values
auto version = root.value("version").unwrap<int>();
// or just 'paste'
string name;
root.paste_field("name", name);
// insert data
root.insert("extensions", { ".cpp", ".cc" });
// access nodes
Node& extensions = root.node("extensions");
extensions.insert(".hpp");
// access by index
auto extension = extensions.value(2).unwrap<string>();
// unwrap nodes
auto vec = extensions.unwrap<vector<string>>();
// translate the tree - to plain bytes...
vector<uint8_t> bytes = tree.to_source<PJson>();
// ...or to a stream
tree.to_source<PJson>(cout); // will print...
``` 
```json
{ 
    "name": "cpp", "version": 11, "extensions": [ ".cpp", ".cc", ".hpp" ]
}
```
Same output as a single expression:
```cpp
Tree().root().insert(
    "name", "cpp",
    "version", 11,
    "extensions", list<string> { ".cpp", ".cc", ".hpp" }
).to_source<PJson>(cout);
```
#### Serializing your types
Implement a resolve method to tell Srl how to handle your types:
```cpp 
struct Lang {
    string name; int version; list<string> extensions;

    void srl_resolve (Srl::Context& ctx) {
        ctx ("name", name) ("version", version) ("extensions", extensions);
    }
};
```
What Context basically does is, depending on ```Srl::Mode```, calling insert or paste on a given ```Srl::Node```.
Mode is either ```::Insert``` or ```::Paste```, you can query the current mode with ```ctx.mode()```.
Taking the vector of bytes from above, you can call:
```cpp
auto lang = Srl::Restore<Lang, PJson>(bytes);
```
#### Handling non-default constructors
Objects are instantiated through a factory ```struct Srl::Ctor<T>```. As default parameterless constructors are required. You can declare
```friend struct Srl::Ctor<YourClass>``` if you don't want to expose public default constructors. Or specialize
```Srl::Ctor<T>``` and define a custom ```T Create()``` method.
#### Handling polymorphic types
```cpp
struct Base {
    int field = 5;
    // declare a srl_type_id method
    virtual const Srl::TypeID& srl_type_id();

    virtual void srl_resolve (Srl::Context& ctx) {
        ctx ("from_base", field);
    }

    virtual ~Base { }
};
// register a type in your implementation.cpp to avoid duplicate registrations of the same type
// this will force a registration on program initialization
const auto base_id = Srl::register_type<Base>("Base");
// return the ID
const Srl::TypeID& Base::srl_type_id() { return base_id; }

// same for derived types
struct Derived : Base {
    int field = 10;

    const Srl::TypeID& srl_type_id() override;

    void srl_resolve (Srl::Context& ctx) override {
        Base::srl_resolve(ctx);
        ctx ("from_derived", field);
    }
};

const auto derived_id = Srl::register_type<Derived>("Derived");
const Srl::TypeID& Derived::srl_type_id() { return derived_id; }

class Composite {
    // make private constructor accessible
    friend struct Srl::Ctor<Composite>;

private:
    Composite() { }
    vector<unique_ptr<Base>> bases;

public:
    Composite (initializer_list<Base*> bases_) {
        for(auto* b : bases_) {
            this->bases.emplace_back(b);
        }
    }
    // cruise control
    void srl_resolve (Srl::Context& ctx) {
        ctx ("bases", bases);
    }

    Base& at (size_t idx) { return *bases[idx].get(); }
};

// running...
Composite composite { new Derived(), new Base() };
Srl::Store<PJson>(cout, composite);
// ...will print...
```
```json
{
    "bases": [
        {
            "srl_type_id": "Derived",
            "from_base": 5,
            "from_derived": 10
        },
        {
            "srl_type_id": "Base",
            "from_base": 5
        }
    ]
}
```
```cpp
/// access a polymorphic type 
auto tree = Tree::From_Type(composite);
auto& bases = tree.root().node("bases");
auto derived = bases.node(0).unwrap<unique_ptr<Base>>();
// or
auto* base = bases.node(1).unwrap<Base*>();
assert(base->srl_type_id().name() == "Base");
// you are responsible for base*
delete base;
// unwrap as Composite
composite = tree.root().unwrap<Composite>();
assert(composite.at(0).srl_type_id().name() == "Derived");
```
#### Handling binary data
Use ```Srl::BitWrap``` to serialize raw binary data...
```cpp
// ...in a srl_resolve method
struct SomeStruct {

  vector<uint8_t> binary;

  void srl_resolve(Context& ctx) {
    BitWrap wrap(
        // Pass a pointer to memory along with the size.
        // This data will be stored on ctx.mode() == Mode::Insert.
        binary.data(), binary.size(),
        // Pass a function f(size)->*mem, the function will be called on ctx.mode() == Mode::Paste
        // with the restored size of the memory block as parameter. The restored data will be
        // copied to the returned pointer.
        [this](size_t size) -> uint8_t* {
            binary.resize(size);
            return binary.data();
        }
    );
    // pass the bit-wrap
    ctx ("data", wrap);
  }
};

// access binary data from a Srl::Tree
static uint8_t binary[] { 2, 4, 6, 8 };
Tree tree;

BitWrap wrap(binary, 4);
tree.root().insert("data", wrap);

wrap = BitWrap( [](size_t sz) { assert(sz == 4); return binary; } );
tree.root().value("data").paste(wrap);
```
For text-based serialization formats binary data will be converted to a base64 string. So calling
```tree.to_source<PJson>(cout)``` on the ```Srl::Tree``` from above will yield...
```json
{
    "data": "AgQGCA=="
}
```

#### Serialization formats
Srl supports 4 serialization formats. Json, Xml, MessagePack, and
[Srl](https://github.com/night-shift/Srl/blob/master/src/lib/PSrl.cpp), a custom space efficient binary format.

Select a format by...
```cpp
// ...passing the corresponding type as a template parameter
auto tree = Tree::From_Source<Srl::PSrl>(source);
// ...or by passing an instance
Srl::PXml xml;
xml.set_compact(true);
auto bytes = tree.to_source(xml);
```
Output encoding for text-based formats is UTF-8. Input is also expected to be UTF-8. As of now no BOM-checking is done, so make sure text documents have the 
correct encoding before parsing.
You can use ```convert_charset``` from ```Srl::Tools::``` for converting to the appropriate character set.

#### Compiler support
At least GCC 4.8 or Clang 3.2 are required. MSVC is lacking some vital C++11 features, so no support as of now.

#### License
MIT License.
