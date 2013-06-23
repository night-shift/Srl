#include "Srl/Registration.h"

using namespace Srl;
using namespace Lib;

Registrations* Lib::registrations()
{
    static Registrations rslt;
    return &rslt;
}
