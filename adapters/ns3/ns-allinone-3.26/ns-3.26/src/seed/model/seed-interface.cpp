#include <string>
#include <map>
#include "./seed-interface.h"

using namespace std;

namespace seed
{
    SeedInterface::SeedInterface(string name, string groups)
        : name (name),
        groups (groups),
        index (99999),
        type ("")
    {}

    string
        SeedInterface::getName()
        {
            return name;
        }


    string
        SeedInterface::getGroups()
        {
            return groups;
        }

    void
        SeedInterface::setIndex(uint32_t index)
        {
            this->index = index;
        }

    uint32_t
        SeedInterface::getIndex()
        {
            return this->index;
        }

    void
        SeedInterface::setType(string type)
        {
            this->type = type;
        }

    string
        SeedInterface::getType()
        {
            return this->type;
        }
}
