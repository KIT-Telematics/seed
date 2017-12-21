#include "seed-parser.h"

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <unordered_map>
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
// using namespace std;
using namespace rapidxml;


namespace seed
{
    SeedParser::SeedParser(string fileName)
        : fileName (fileName)
    {}

    unordered_map<string, SeedNode>&
        SeedParser::getNodes()
        {
            return this->seedNodes;
        }

    unordered_map<string, SeedNodeGroup>&
        SeedParser::getNodeGroups()
        {
            return this->seedNodeGroups;
        }

    unordered_map<string, SeedLink>&
        SeedParser::getLinks()
        {
            return this->seedLinks;
        }

    unordered_map<string, SeedLinkGroup>&
        SeedParser::getLinkGroups()
        {
            return  this->seedLinkGroups;
        }

    // unordered_map<uint32_t, SeedInterface>&
    //     SeedParser::getInterfaces()
    //     {
    //         return this->seedInterfaces;
    //     }

    unordered_map<string, SeedInterfaceGroup>&
        SeedParser::getInterfaceGroups()
        {
            return this->seedInterfaceGroups;
        }

    vector<shared_ptr<SeedEvent>>&
        SeedParser::getEvents()
        {
            return this->seedEvents;
        }


    void
        SeedParser::parse()
        {
            this->initParse();
            this->parseInterfaceGroups(this->rootNode);
            this->parseLinkGroups(this->rootNode);
            this->parseNodeGroups(this->rootNode);
            this->parseNodes(this->rootNode);
            this->parseSchedule(this->rootNode);
            this->parseLinks(this->rootNode);
            // this->parseInterfaces(this->rootNode);
        }

    void
        SeedParser::initParse()
        {
            ifstream fin( this->fileName );
            ostringstream sstr;
            sstr << fin.rdbuf();
            sstr.flush();
            fin.close();

            source = sstr.str();
            rapidxml::xml_document<> doc;
            doc.parse<0> (&source[0]);

            // rapidxml::xml_node<> *rootNode = doc.first_node("bundle");
            this->rootNode = doc.first_node("bundle");

            if(!this->rootNode){
                // TODO print to stderr
                std::cout << "Error reading XML-File \"" << this->fileName << "\"" << std::endl;
                std::cout << "Stop." << std::endl;
            }
            std::cout << std::endl;

            string bundleName = this->getAttribute(this->rootNode, "name");
            if(bundleName == ""){
                std::cout << "Error: Bundle has no name" << std::endl;
            }
            std::cout << "  ==> Initializing bundle \"" << bundleName << "\"" << std::endl;
            std::cout << std::endl;

            std::cout << "===============================================" << std::endl;
            std::cout << "==> SEED: Parsing Bundle" << std::endl;
        }


    // TODO save params
    // void SeedParser::parseParams(xml_node<> *rootNode)
    // {
    // //std::cout << "===============================================" << std::endl;
    // //std::cout << "==> SEED: Parsing Parameters" << std::endl;
    // // NS_LOG_INFO ("Parsing Parameters.");
    // // for (xml_node<> * tmpNode = rootNode->first_node("parameters")->first_node("parameter"); tmpNode; tmpNode = tmpNode->next_sibling())
    // // {
    // //     string key = tmpNode->first_attribute("name")->value();
    // //     string value = tmpNode->first_attribute("value")->value();
    // //    std::cout << "  --> Creating Parameter \"" << key << "\" with value \"" << value << "\"" << std::endl;
    // // }
    // //std::cout << std::endl;
    // }






    void
        SeedParser::parseNodeGroups(xml_node<> *rootNode)
        {
            for (xml_node<> * tmpNode = rootNode->first_node("topology")->first_node("nodegroups")->first_node("group"); tmpNode; tmpNode = tmpNode->next_sibling())
            {
                string tmpName = this->getAttribute(tmpNode, "name");
                SeedNodeGroup sng = SeedNodeGroup(tmpName);

                if (tmpNode->first_attribute("type"))
                {
                    NodeType tmpNodeType = NodeType::HOST;
                    string tmpType = tmpNode->first_attribute("type")->value();
                    if(tmpType == "ofSwitch"){
                        tmpNodeType = NodeType::OF_SWITCH;
                    }else if(tmpType == "ofController"){
                        tmpNodeType = NodeType::OF_CONTROLLER;
                    }else if(tmpType == "genericSwitch"){
                        tmpNodeType = NodeType::GENERIC_SWITCH;
                    }else if(tmpType == "host"){
                        tmpNodeType = NodeType::HOST;
                    }else{
                        std::cout << "Error undefinded NodeGroup-Type \"" << tmpType << "\"" << std::endl;
                        std::cout << "Stop." << std::endl;
                    }
                    sng.setType(tmpNodeType);
                }
                //std::cout << "Parsed NodeGroup: " << sng.getName() << std::endl;
                this->seedNodeGroups.insert(std::make_pair(tmpName, sng));
            }
            std::cout << "  ==> " << this->seedNodeGroups.size() << " NodeGroups parsed" << std::endl;
        }


    void
        SeedParser::parseNodes(xml_node<> *rootNode)
        {
            uint32_t index = 0;
            for (xml_node<> * tmpNode = rootNode->first_node("topology")->first_node("nodes")->first_node("node"); tmpNode; tmpNode = tmpNode->next_sibling())
            {
                string tmpName = this->getAttribute(tmpNode, "name");

                string tmpGroups = "";
                if (tmpNode->first_attribute("groups"))
                {
                    tmpGroups = tmpNode->first_attribute("groups")->value();
                }

                NodeType tmpNodeType = NodeType::HOST;
                if (tmpNode->first_attribute("type"))
                {
                    string tmpType = tmpNode->first_attribute("type")->value();
                    if(tmpType == "ofSwitch"){
                        tmpNodeType = NodeType::OF_SWITCH;
                    }else if(tmpType == "ofController"){
                        tmpNodeType = NodeType::OF_CONTROLLER;
                    }else if(tmpType == "genericSwitch"){
                        tmpNodeType = NodeType::GENERIC_SWITCH;
                    }else{
                        tmpNodeType = NodeType::HOST;
                    }
                }

                // TODO:
                //  parse node's "groups"-attribute
                std::string s = tmpGroups;
                for(auto const &it : split(tmpGroups, " "))
                {
                    auto itAccess = this->seedNodeGroups.find(it);
                    if (itAccess != this->seedNodeGroups.end())
                    {
                        // node has group (which exists)
                        if(itAccess->second.getType() != NodeType::UNDEFINED)
                        {
                            tmpNodeType = itAccess->second.getType();
                            //std::cout << "true" << std::endl;
                        }
                        //std::cout << "node has existing group " << it <<  std::endl;
                    }
                }

                auto nodeCopy = tmpNode;
                auto interfaces = this->parseInterfaces(nodeCopy);
                // for(auto const& si : interfaces)
                // {
                //     // auto si = &interfacesIt.second;
                //     // this->seedInterfaces.insert(std::make_pair(si->getName(), si));
                //     // cout << "  ---> with Interface: " << si->getName() << endl;
                // }

                SeedNode sn = SeedNode(tmpName, tmpGroups, tmpNodeType);

                sn.setInterfaces(interfaces);

                if (tmpNode->first_attribute("pos-x"))
                {
                    sn.setPosX( stod( tmpNode->first_attribute("pos-x")->value() ) );
                }

                if (tmpNode->first_attribute("pos-y"))
                {
                    sn.setPosY( stod( tmpNode->first_attribute("pos-y")->value() ) );
                }

                string controllerName = this->getAttribute(tmpNode, "controller");
                sn.setControllerName(controllerName);

                this->seedNodes.insert(std::make_pair(tmpName, sn));
                index++;
            }
            std::cout << "  ==> " << this->seedNodes.size() << " Nodes parsed" << std::endl;
            //std::cout << "  ===> with " << this->seedInterfaces.size() << " Interfaces parsed" << std::endl;
        }


    void
        SeedParser::parseLinkGroups(xml_node<> *rootNode)
        {
            for (xml_node<> * tmpNode = rootNode->first_node("topology")->first_node("linkgroups")->first_node("group"); tmpNode; tmpNode = tmpNode->next_sibling())
            {
                string tmpName = this->getAttribute(tmpNode, "name");
                SeedLinkGroup sng = SeedLinkGroup(tmpName);
                //std::cout << "Parsed LinkGroup: " << sng.getName() << std::endl;
                this->seedLinkGroups.insert(std::make_pair(tmpName, sng));
            }
            std::cout << "  ==> " << this->seedLinkGroups.size() << " LinkGroups parsed" << std::endl;
        }


    void
        SeedParser::parseLinks(xml_node<> *rootNode)
        {
            for (xml_node<> * tmpNode = rootNode->first_node("topology")->first_node("links")->first_node("link"); tmpNode; tmpNode = tmpNode->next_sibling())
            {
                string fooType = tmpNode->name();
                // string tmpName = this->getAttribute(tmpNode, "name");
                // std::cout << "blub: " << fooType << std::endl;
                string tmpName = tmpNode->first_attribute("name")->value();

                string tmpGroups = "";
                if (tmpNode->first_attribute("groups"))
                {
                    tmpGroups = tmpNode->first_attribute("groups")->value();
                }

                string source = "";
                if (tmpNode->first_attribute("a"))
                {
                    source = tmpNode->first_attribute("a")->value();
                }

                string destination = "";
                if (tmpNode->first_attribute("b"))
                {
                    destination = tmpNode->first_attribute("b")->value();
                }

                string bandwidth = "";
                if (tmpNode->first_attribute("bandwidth"))
                {
                    bandwidth = tmpNode->first_attribute("bandwidth")->value();
                }

                string delay = "";
                if (tmpNode->first_attribute("delay"))
                {
                    delay = tmpNode->first_attribute("delay")->value();
                }

                double errorRate = 0;
                if (tmpNode->first_attribute("error-rate"))
                {
                    errorRate = (double) (stod( tmpNode->first_attribute("error-rate")->value() ) / 100);
                }

                string state = "";
                if (tmpNode->first_attribute("state"))
                {
                    state = tmpNode->first_attribute("state")->value();
                }

                // TODO: inherit group-props
                // //  parse node's "groups"-attribute
                // std::string s = tmpGroups;
                // for(auto const &it : split(tmpGroups, " "))
                // {
                //     auto itAccess = this->seedNodeGroups.find(it);
                //     if (itAccess != this->seedNodeGroups.end())
                //     {
                //         // node has group (which exists)
                //         if(itAccess->second.getType() != NodeType::UNDEFINED)
                //         {
                //             tmpNodeType = itAccess->second.getType();
                //            std::cout << "true" << std::endl;
                //         }
                //        std::cout << "node has existing group " << it <<  std::endl;
                //     }
                // }
                SeedLink sl = SeedLink(tmpName, tmpGroups, source, destination, bandwidth, delay, errorRate, state);
                this->seedLinks.insert(std::make_pair(tmpName, sl));
            }
            std::cout << "  ==> " << this->seedLinks.size() << " Links parsed" << std::endl;
        }


    vector<shared_ptr<SeedInterface>>
        SeedParser::parseInterfaces(xml_node<> *rootNode)
        {
            vector<shared_ptr<SeedInterface>> nodesSeedInterfaces;
            for (xml_node<> * tmpNode = rootNode->first_node("interface"); tmpNode; tmpNode = tmpNode->next_sibling())
            {
                string tmpName = this->getAttribute(tmpNode, "name");

                string tmpGroups = "";
                if (tmpNode->first_attribute("groups"))
                {
                    tmpGroups = tmpNode->first_attribute("groups")->value();
                }

                string type = this->getAttribute(tmpNode, "type");

                // SeedInterface si = SeedInterface(tmpName, tmpGroups);
                // std::cout << "Parsed Interface: " << tmpName << " with group \"" << tmpGroups << "\"" << std::endl;
                // nodesSeedInterfaces.insert(std::make_pair(tmpName, si));
                shared_ptr<SeedInterface>  interfacePtr(new SeedInterface(tmpName, tmpGroups));
                interfacePtr->setType(type);
                nodesSeedInterfaces.push_back( interfacePtr );
                // nodesSeedInterfaces.push_back( si );
            }
            return nodesSeedInterfaces;
        }


    void
        SeedParser::parseInterfaceGroups(xml_node<> *rootNode)
        {
            for (xml_node<> * tmpNode = rootNode->first_node("topology")->first_node("interfacegroups")->first_node("group"); tmpNode; tmpNode = tmpNode->next_sibling())
            {
                string tmpName = this->getAttribute(tmpNode, "name");
                SeedInterfaceGroup sng = SeedInterfaceGroup(tmpName);
                //std::cout << "Parsed InterfaceGroup: " << sng.getName() << std::endl;
                this->seedInterfaceGroups.insert(std::make_pair(tmpName, sng));
            }
            std::cout << "  ==> " << this->seedInterfaceGroups.size() << " InterfaceGroups parsed" << std::endl;
        }


    void
        SeedParser::parseSchedule(xml_node<> *rootNode)
        {
            for (xml_node<> * tmpNode = rootNode->first_node("schedule")->first_node(); tmpNode; tmpNode = tmpNode->next_sibling())
            {
                string eventType = tmpNode->name();
                double startTime = 0;
                if(this->getAttribute(tmpNode, "start") != ""){
                    startTime = stod(this->getAttribute(tmpNode, "start"));
                }
                //std::cout << "eventType " << eventType << std::endl;
                //std::cout << "event " << startTime << std::endl;

                if(eventType == "bulk-event"){
                    //std::cout << "bulk event" << std::endl;
                    string source = tmpNode->first_attribute("source")->value();
                    string destination  = tmpNode->first_attribute("destination")->value();
                    string sizeAsString = tmpNode->first_attribute("max-size")->value();
                    long size = 0;
                    SeedParser::parseSize(sizeAsString, size);
                    //std::cout << "a: " << sizeAsString << " b: " << size << std::endl;
                    shared_ptr<SeedBulkTrafficEvent> sbtePtr(new SeedBulkTrafficEvent(EventType::TRAFFIC_BULK, startTime, source, destination, size));
                    this->seedEvents.push_back( sbtePtr );
                }else if(eventType == "link-state-change-event"){
                    //std::cout << "link event" << std::endl;
                    string link = tmpNode->first_attribute("link")->value();
                    string newStateString  = tmpNode->first_attribute("state")->value();
                    bool newState  = (newStateString == "on") ? true : false;
                    shared_ptr<SeedLinkChangeEvent>  slcePtr(new SeedLinkChangeEvent(EventType::LINK_CHANGE_STATE, startTime, link, newState));
                    this->seedEvents.push_back( slcePtr );
                }else{
                    // TODO error handling
                    std::cout << "Error: Unknown event-type!" << std::endl;
                }
            }
            std::cout << "  ==> " << this->seedEvents.size() << " events parsed" << std::endl;
        }


    vector<string>
        SeedParser::split(const string& str, const string& delim)
        {
            vector<string> tokens;
            size_t prev = 0, pos = 0;
            do
            {
                pos = str.find(delim, prev);
                if (pos == string::npos) pos = str.length();
                string token = str.substr(prev, pos-prev);
                if (!token.empty()) tokens.push_back(token);
                prev = pos + delim.length();
            }
            while (pos < str.length() && prev < str.length());
            return tokens;
        }

    std::pair<std::string, std::string>
        SeedParser::splitNumber(std::string str)
        {
            auto it = str.begin();
            for (; it != str.end() && (('0' <= *it && *it <= '9') || *it == '.'); ++it);
            return std::make_pair(std::string(str.begin(), it),
                    std::string(it, str.end()));
        }

    bool
        SeedParser::parseSize(std::string size, long &res)
        {
            auto pair = SeedParser::splitNumber(size);
            if (!(std::istringstream(pair.first) >> res))
                return false;

            if (pair.second == "Tbyte")
                res *= (long) (1025*1024) * (long) (1024*1024);
            else if (pair.second == "Gbyte")
                res *= 1024*1024*1024;
            else if (pair.second == "Mbyte")
                res *= 1024*1024;
            else if (pair.second == "Kbyte")
                res *= 1024;
            else if (pair.second == "byte")
                res *= 1;
            else return false;

            return true;
        }


    std::string SeedParser::getAttribute(rapidxml::xml_node<>* node,
            const std::string &name) {
        rapidxml::xml_attribute<> *attr = node->first_attribute(name.c_str());
        if(attr){
            return attr->value();
        }else{
            return std::string();
        }
    }
}
