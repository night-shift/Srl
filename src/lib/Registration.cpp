#include "Srl/Registration.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

Registrations* Lib::registrations()
{
    static Registrations rslt;
    return &rslt;
}
bool Registrations::insert(const String& id, const function<void*(void)>& fnc)
{
    bool exists; void* v;
    tie(exists, v) = this->table.insert(id, fnc);

    return exists;
}

function<void*(void)>* Registrations::find (const String& id)
{
    return this->table.get(id);
}
