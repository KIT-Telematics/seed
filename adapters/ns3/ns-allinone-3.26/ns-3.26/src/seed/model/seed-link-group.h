#ifndef LINK_GROUP_H
#define LINK_GROUP_H value

#include <string>
#include <map>
#include "seed-node-types.h"

using namespace std;

namespace seed
{
    class SeedLinkGroup
    {
        public:
            SeedLinkGroup(string name);
            string getName();

        protected:
            string name;

    };
}
#endif /* ifndef LINK_GROUP_H */
