## Srl
### Serializes.
#### Short story
```cpp
#include "Srl/Srl.h"

class YourClass {

    int m_int = 10;  // handles basic data types
    u32string m_string = U"string";  // strings
    vector<double> m_vector = { 1.0, 2.0, 3.0 };  // aswell as stl-containers
    map<string, short> m_map = { { "A", 0 }, { "B", 1 } };  // including maps

public :
    // implement a srl_resolve method
    template<Srl::Mode M>
    void srl_resolve(Srl::Context<M>& ctx) {
        // tell the context what to serialize, field names are optional
        ctx ("m_int", m_int)
            ("m_string", m_string)
            ("m_vector", m_vector)
            ("m_map", m_map);
    }
};

int main() {
    YourClass original;
    // serialize with Srl::Store
    vector<uint8_t> bytes = Srl::Store<Srl::PSrl>(original);
    //                                     |-> choose a parser
    YourClass restored;
    // deserialize with Srl::Restore
    Srl::Restore<Srl::PSrl>(restored, bytes);
    // Thats it.
    return 0;
}
```
### Long Story
#### Process documents
```cpp
string json = "{ "
    "\"name\"    : \"cpp\","
    "\"version\" : 11"
"}";
// create Srl::Tree from a source
Tree tree = Tree::From_Source<PJson>(json.c_str(), json.size());
// the root node
Node* root = tree.root();
// access values
auto version = root->value("version")->unwrap<int>();
// or just 'paste'
string name;
root->paste_field("name", name);
// insert data
root->insert("extensions", { ".cpp", ".cc" });
// access nodes
Node* extensions = root->node("extensions");
extensions->insert(".hpp");
// access by index
auto extension = extensions->value(2)->unwrap<string>();
// or just unwrap the whole node
auto vec = extensions->unwrap<vector<string>>();
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
Tree().root()->insert(
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

    template<Mode M> void srl_resolve (Context<M>& ctx) {
        ctx ("name", name) ("version", version) ("extensions", extensions);
    }
};
```
What Context basically does is, depending on Mode, calling insert or paste on a given Srl::Node.
Mode is either ::Insert or ::Paste, specialize if you want something different to be done for each case.
Taking the vector of bytes from above, you can call:
```cpp
auto lang = Srl::Restore<Lang, PJson>(bytes);
```
Which is actually the short form for:
```cpp
auto tree = Tree::From_Source<PJson>(bytes);
auto lang = tree.root()->unwrap<Lang>();
```
#### Handling binary data
Use Srl::BitWrap helper to serialize raw binary data:
```cpp
// in a srl_resolve method
struct SomeStruct {

    vector<uint8_t> data;

    template<Mode M> void srl_resolve(Context<M>& ctx) {
        BitWrap wrap(
            // Pass a pointer to memory along with the size.
            // This data will be stored on Mode M == Mode::Insert.
            &data[0], data.size(),
            // Pass a function f(size)->*mem, the function will be called on Mode M == Mode::Paste
            // with the restored size of the memory block as parameter. The restored data will be
            // copied to the returned pointer.
            [this](size_t size) -> uint8_t* {
                data.resize(size);
                return &data[0];
            }
        );
        // pass the bit-wrap
        ctx ("data", wrap);
    }
};

// directly access binary data from a tree
static uint8_t binary[] { 2, 4, 6, 8 };
Tree tree;

BitWrap wrap_insert(binary, 4);
tree.root()->insert("data", wrap_insert);

BitWrap wrap_paste( [](size_t sz) { assert(sz == 4); return binary; } );
tree.root()->value("data")->paste(wrap_paste);
```
For text-based serialization formats binary data will be converted to a base64 string. So calling
```tree.to_source<PJson>(cout)``` on the Srl::Tree from above will yield ```{ "data": "AgQGCA==" }```.

#### Serialization formats
Srl supports 4 serialization formats. Json, Xml, Bson and [Srl](https://github.com/night-shift/Srl/blob/master/src/lib/PSrl.cpp), a custom space efficient binary format.

Select a format by...
```cpp
// ...passing the corresponding type as a template parameter
auto tree = Tree::From_Source<PBson>(source);
// ...or by passing an instance
PXml xml;
xml.set_skip_whitespace(true);
tree.to_source(xml);
```
Output encoding for text-based formats is UTF-8. Input is also expected to be UTF-8. As of now no BOM-checking is done, so make sure text documents have the 
correct encoding before parsing. 
You can use ```convert_charset``` from Srl::Tools:: for converting to the appropriate character set.

#### Compiler support
At least gcc 4.8 or clang 3.2 is needed. I recommend using gcc since it produces significantly faster code. Especially json/xml parsers
lead up to a two-fold performance increase.

At time of writing MSVC is missing some vital C++11 features for compiling the project.
