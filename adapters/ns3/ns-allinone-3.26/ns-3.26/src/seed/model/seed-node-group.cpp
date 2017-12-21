#include <string>
#include <map>
#include "seed-node-types.h"
#include "seed-node-group.h"

using namespace std;

namespace seed
{
    SeedNodeGroup::SeedNodeGroup(string name)
        : name (name),
        nodeType (NodeType::UNDEFINED)
    {}

    string
        SeedNodeGroup::getName()
        {
            return name;
        }

    void
        SeedNodeGroup::setType(NodeType nodeType)
        {
            nodeType = nodeType;
        }

    NodeType
        SeedNodeGroup::getType()
        {
            return nodeType;
        }


    void
        SeedNodeGroup::setControllerName(string controllerName)
        {
            controllerName = controllerName;
        }

    string
        SeedNodeGroup::getControllerName(string controllerName)
        {
            return controllerName;
        }
}
