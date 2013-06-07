# Srl

### Serializes.

## Short story

```cpp
#include "Srl/Srl.h"

using namespace std;

class YourClass {
    // handles basic datatypes
    int m_int = 10;
    // strings
    u32string m_string = U"string";
    // aswell as most stl-containers
    vector<double> m_vector = { 1.0, 2.0, 3.0 };
    // including maps
    map<string, short> m_map = { { "A", 0 }, { "B", 1 } };
public :
    // just define a srl_resolve method
    template<Srl::Mode M>
    void srl_resolve(Srl::Context<M>& ctx)
    {
        // feed the context, field specifiers are optional
        ctx (m_int, "m_int")
            (m_string, "m_string")
            (m_vector, "m_vector")
            (m_map, "m_map");
    }
};

int main()
{
    YourClass original;
    // serialize with Srl::Store
    vector<uint8_t> bytes = Srl::Store<Srl::PSrl>(original);
    //                                     |-> choose a parser
    YourClass restored;
    // deserialize with Srl::Restore
    Srl::Restore<Srl::PSrl>(restored, bytes);

    // Thats it. Simple as that.
    return 0;
}
```
