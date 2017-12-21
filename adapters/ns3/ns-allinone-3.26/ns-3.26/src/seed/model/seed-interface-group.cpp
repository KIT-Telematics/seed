#include <string>
#include <map>
#include "seed-interface-group.h"

using namespace std;

namespace seed
{
    SeedInterfaceGroup::SeedInterfaceGroup(string name)
        : name (name)
    {}

    string
        SeedInterfaceGroup::getName()
        {
            return name;
        }
}
