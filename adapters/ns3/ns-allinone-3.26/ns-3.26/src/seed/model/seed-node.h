#ifndef SEED_NODE_H
#define SEED_NODE_H value

#include <string>
#include <map>
#include <vector>
#include <unordered_map>
#include <memory>
#include "seed-node-types.h"
#include "seed-interface.h"

using namespace std;

namespace seed
{
    class SeedNode
    {
        public:
            SeedNode(string name, string groups, NodeType nodeType);
            string getName();
            string getControllerName();
            string getGroups();
            NodeType getType();
            void setInterfaces(vector<shared_ptr<SeedInterface>> nodesSeedInterfaces);
            vector<shared_ptr<SeedInterface>> getInterfaces();
            void setIndex(uint32_t index);
            uint32_t getIndex();
            double getPosX();
            double getPosY();
            void setPosX(double posX);
            void setPosY(double posY);
            void setControllerName(string controllerName);

        protected:
            string name;
            string controllerName = "controller1";
            string groups;
            NodeType nodeType;
            vector<shared_ptr<SeedInterface>> interfaces;
            uint32_t index;
            double posX;
            double posY;

    };
}
#endif /* ifndef SEED_NODE_H */
