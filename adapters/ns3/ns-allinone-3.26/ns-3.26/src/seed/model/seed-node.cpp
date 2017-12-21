#include <string>
#include <map>
#include "seed-node-types.h"
#include "seed-node.h"

using namespace std;

namespace seed
{
    SeedNode::SeedNode(string name, string groups, NodeType nodeType)
        : name (name),
        groups (groups),
        nodeType (nodeType),
        posX (0),
        posY (0)
    {}

    string
        SeedNode::getName()
        {
            return this->name;
        }


    string
        SeedNode::getGroups()
        {
            return this->groups;
        }

    NodeType
        SeedNode::getType()
        {
            return this->nodeType;
        }

    void
        SeedNode::setInterfaces(vector<shared_ptr<SeedInterface>> nodesSeedInterfaces)
        {
            this->interfaces = nodesSeedInterfaces;
        }

    vector<shared_ptr<SeedInterface>>
        SeedNode::getInterfaces()
        {
            return this->interfaces;
        }

    void
        SeedNode::setIndex(uint32_t index)
        {
            this->index = index;
        }

    uint32_t
        SeedNode::getIndex()
        {
            return this->index;
        }

    void
        SeedNode::setPosX(double posX)
        {
            this->posX = posX;
        }

    void
        SeedNode::setPosY(double posY)
        {
            this->posY = posY;
        }

    string
        SeedNode::getControllerName()
        {
            return this->controllerName;
        }

    void
        SeedNode::setControllerName(string controllerName)
        {
            this->controllerName = controllerName;
        }

    double
        SeedNode::getPosX()
        {
            return this->posX;
        }

    double
        SeedNode::getPosY()
        {
            return this->posY;
        }
}
