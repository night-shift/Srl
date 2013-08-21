Srl
=====
```cpp
#include "Srl/Srl.h"

class YourClass {
    // handles basic data types
    int a = 10;
    // strings
    u32string b = U"string";
    // stl-containers
    vector<double> c { 1.0, 2.0, 3.0 };
    // shared_ / unique_ptrs
    shared_ptr<int> d = make_shared<int>(5);
    // tuples
    tuple<bool, string, char> e { false, "tuple", 'N' };

public :
    // implement a srl_resolve method
    void srl_resolve (Srl::Context& ctx) {
        // tell the context what to serialize, field names are optional
        ctx ("a", a) ("b", b) ("c", c)
            ("d", d) ("e", e);
    }
};

int main() {
    YourClass original;
    // serialize with Tree::store
    Srl::Tree tree;
    vector<uint8_t> bytes = tree.store<Srl::PJson>(original);
    //                                     |-> choose a parser
    // deserialize with Tree::restore
    YourClass restored = tree.restore<YourClass, Srl::PJson>(bytes);
    // or use streams
    ofstream fso("file");
    tree.store<Srl::PMsgPack>(original, fso);

    ifstream fsi("file");
    tree.restore<Srl::PMsgPack>(restored, fsi);
    // Thats it.
    return 0;
}
```
##Overview
#### Processing documents
```cpp
string json = "{ "
    "\"name\"    : \"cpp\","
    "\"version\" : 11"
"}";
// load a document
Tree tree;
tree.load_source<PJson>(json.c_str(), json.size());
// the root node
Node& root = tree.root();
// access values
auto version = root.value("version").unwrap<int>();
// or just 'paste'
string name;
root.value("name").paste(name);
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
Same output as a single expression
```cpp
Tree().root().insert(
    "name", "cpp",
    "version", 11,
    "extensions", list<string> { ".cpp", ".cc", ".hpp" }
).to_source<PJson>(cout);
```
#### Serializing your types
To tell Srl how to handle your types implement either a single resolve method
```cpp
void srl_resolve (Srl::Context& ctx) { ... }
```
or dedicated insert and paste methods
```cpp
void srl_insert (Srl::InsertContext& ctx) const { ... }
void srl_paste  (Srl::PasteContext& ctx) { ... }
```

```cpp
struct Lang {
    string name; int version; list<string> extensions;

    void srl_resolve (Srl::Context& ctx) {
        // Srl is order-agnostic
        ctx ("version", version) ("extensions", extensions) ("name", name);
    }
};
```

What Context does is, depending on ```Srl::Mode```, calling insert or paste on a given ```Srl::Node```.
Mode is either ```::Insert``` or ```::Paste```, you can query the current mode with ```ctx.mode()```.
Taking the vector of bytes from above you can call:
```cpp
auto lang = Tree().restore<Lang, PJson>(bytes);
```
#### Handling non-default constructors
Objects are instantiated through a factory ```struct Srl::Ctor<T>```. As default parameterless constructors are required. You can declare
```friend struct Srl::Ctor<YourClass>``` if you don't want to expose public default constructors. Or specialize
```Srl::Ctor<T>``` and define a custom ```T Create()``` method.
#### Handling polymorphic types
```cpp
struct Base {
    // declare a srl_type_id method
    virtual const Srl::TypeID& srl_type_id();

    virtual void srl_resolve (Srl::Context& ctx) {
        ctx ("from_base", 5);
    }

    virtual ~Base { }
};
// register a type in your implementation.cpp to avoid duplicate
// registrations of the same type
// no RTTI needed
const auto base_id = Srl::register_type<Base>("Base");
// return the ID
const Srl::TypeID& Base::srl_type_id() { return base_id; }

// same for derived types
struct Derived : Base {

    const Srl::TypeID& srl_type_id() override;

    void srl_resolve (Srl::Context& ctx) override {
        Base::srl_resolve(ctx);
        ctx ("from_derived", 10);
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
Tree().store<PJson>(composite, cout);
// ...will print:
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
Tree tree;
tree.load_object(composite);
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
#### Handling shared references
Simply use ```std::shared_ptr / std::weak_ptr```
```cpp
auto first  = make_shared<Lang>("objc", 2, list<string> { ".h", ".m", ".mm" });
auto second = first;
Tree().root().insert("first", first, "second", second)
             .to_source<PJson>(cout); // yields...
```
```json
{
    "first": {
		"srl_shared_key": 0,
        "srl_shared_value": {
            "name": "objc",
            "version": 2,
            "extensions": [ ".h", ".m", ".mm" ]
        }
	},
	"second": {
		"srl_shared_key": 0
	}
}
```
#### Handling binary data
Use ```Srl::BitWrap / Srl::VecWrap``` to serialize raw binary data
```cpp
// in a srl_resolve method
struct SomeStruct {

  vector<uint8_t> binary;

  void srl_resolve(Context& ctx) {
    BitWrap wrap(
        // Pass a pointer to memory along with the size.
        // This data will be stored on ctx.mode() == Mode::Insert.
        binary.data(), binary.size(),
        // Pass a function f(size)->*mem, the function will be called on
        // ctx.mode() == Mode::Paste with the restored size of the
        // memory block as parameter. The restored data will be
        // copied to the returned pointer.
        [this](size_t size) -> uint8_t* {
            binary.resize(size);
            return binary.data();
        }
    );
    // pass the bit-wrap
    ctx ("data", wrap);

    // or just use the shortcut for serializing vectors as binary blobs
    ctx ("datavec", VecWrap<uint8_t>(binary));
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
For text-based serialization formats binary data will be converted to a base64 string. Calling
```tree.to_source<PJson>(cout)``` on the ```Srl::Tree``` from above will yield:
```json
{
    "data": "AgQGCA=="
}
```

#### Serialization formats
Srl supports 4 serialization formats:
* Json
* Xml
* MessagePack
* [Srl](https://github.com/night-shift/Srl/blob/master/src/lib/PSrl.cpp), a custom space efficient binary format.

Select a format by...
```cpp
// ...passing the corresponding type as a template parameter
Tree tree;
tree.load_source<Srl::PSrl>(source);
// ...passing an instance
Srl::PXml xml;
xml.set_compact(true);
auto bytes = tree.to_source(xml);
```
Output encoding for text-based formats is UTF-8. Input is also expected to be UTF-8. As of now no BOM-checking is done, so make sure text documents have the 
correct encoding before parsing.
You can use ```convert_charset``` from ```Srl::Tools::``` for converting to the appropriate character set.

#### Supported compilers
At least GCC 4.8 or Clang 3.2 are required. MSVC is lacking some vital C++11 features, so no support as of now.

#### License
MIT License.
