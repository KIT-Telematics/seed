#ifndef INTERFACE_H
#define INTERFACE_H

#include <string>
#include <map>

using namespace std;

namespace seed
{
    class SeedInterface
    {
        public:
            SeedInterface(string name, string groups);
            string getName();
            string getGroups();
            string getType();
            void setType(string type);
            void setIndex(uint32_t index);
            uint32_t getIndex();

        protected:
            string name;
            string groups;
            uint32_t index;
            string type;
    };
}
#endif // INTERFACE_H
