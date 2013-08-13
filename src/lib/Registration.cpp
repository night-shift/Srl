#include "Srl/Registration.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;


Registrations* Lib::registrations()
{
    static Registrations regs;
    return &regs;
}

bool Registrations::insert(const String& id, Registrations::Make&& make)
{
    bool exists; void* v;
    tie(exists, v) = this->table.insert(id, make);

    return exists;
}

Registrations::Make* Registrations::find (const String& id)
{
    return this->table.get(id);
}
