#ifndef INTERFACE_GROUP_H
#define INTERFACE_GROUP_H

#include <string>
#include <map>

using namespace std;

namespace seed
{
    class SeedInterfaceGroup
    {
        public:
            SeedInterfaceGroup(string name);
            string getName();

        protected:
            string name;

    };
}
#endif // INTERFACE_GROUP_H

