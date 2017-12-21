#ifndef NODE_GROUP_H
#define NODE_GROUP_H value

#include <string>
#include <map>
#include "seed-node-types.h"

using namespace std;

namespace seed
{
    class SeedNodeGroup
    {
        public:
            SeedNodeGroup(string name);
            string getName();
            void setType(NodeType nodeType);
            NodeType getType();
            void setControllerName(string controllerName);
            string getControllerName(string controllerName);

        protected:
            string name;
            NodeType nodeType;
            string controllerName;

    };
}
#endif /* ifndef NODE_GROUP_H */
