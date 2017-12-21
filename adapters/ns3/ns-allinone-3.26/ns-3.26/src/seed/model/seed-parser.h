#ifndef PARSER_H
#define PARSER_H value

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "rapidxml.hpp"
#include "seed-interface.h"
#include "seed-interface-group.h"
#include "seed-node-types.h"
#include "seed-node.h"
#include "seed-node-group.h"
#include "seed-link.h"
#include "seed-link-group.h"
#include "seed-event-types.h"
#include "seed-event.h"
#include "seed-event-traffic-bulk.h"
#include "seed-event-link-change.h"

using namespace seed;
using namespace rapidxml;

namespace seed
{
    class SeedParser
    {
        public:
            typedef std::unordered_map <std::string, SeedNode> nodeMap;
            typedef std::unordered_map <std::string, SeedNodeGroup> nodeGroupMap;
            typedef std::unordered_map <std::string, SeedLink> linkMap;
            typedef std::unordered_map <std::string, SeedLinkGroup> linkGroupMap;
            typedef std::unordered_map <std::string, SeedInterface> interfaceMap;
            typedef std::unordered_map <std::string, SeedInterfaceGroup> interfaceGroupMap;
            typedef vector<shared_ptr<SeedEvent>> eventCollection;
            typedef vector<shared_ptr<SeedInterface>> interfaceCollection;
            SeedParser(string fileName);
            nodeMap& getNodes();
            nodeGroupMap& getNodeGroups();
            linkMap& getLinks();
            linkGroupMap& getLinkGroups();
            // unordered_map<string, SeedInterface>& getInterfaces();
            interfaceGroupMap& getInterfaceGroups();
            eventCollection& getEvents();
            void parse();


        protected:
            string fileName;
            string source;
            rapidxml::xml_node<> *rootNode;
            nodeMap seedNodes;
            nodeGroupMap seedNodeGroups;
            linkMap seedLinks;
            linkGroupMap seedLinkGroups;
            interfaceMap seedInterfaces;
            interfaceGroupMap seedInterfaceGroups;
            eventCollection seedEvents;
            void initParse();
            void parseNodeGroups(xml_node<> *rootNode);
            void parseNodes(xml_node<> *rootNode);
            void parseLinks(xml_node<> *rootNode);
            void parseLinkGroups(xml_node<> *rootNode);
            interfaceCollection parseInterfaces(xml_node<> *rootNode);
            void parseInterfaceGroups(xml_node<> *rootNode);
            void parseSchedule(xml_node<> *rootNode);

        private:
            vector<string> split(const string& str, const string& delim);
            bool parseSize(std::string size, long &res);
            long fileSizeAsStringToBytes (std::string size);
            std::pair<std::string, std::string> splitNumber(std::string str);
            std::string getAttribute(rapidxml::xml_node<>* node, const std::string &name);

    };

}
#endif /* ifndef PARSER_H */
