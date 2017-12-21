#include <string>
#include <map>
#include "seed-link-group.h"
#include "seed-node-types.h"

using namespace std;

namespace seed
{
    SeedLinkGroup::SeedLinkGroup(string name)
        : name (name)
    {}

    string
        SeedLinkGroup::getName()
        {
            return name;
        }
}
