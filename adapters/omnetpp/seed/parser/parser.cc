#include "parser/parser.h"

#include "expat.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <omnetpp.h>
#include "parser/internal.h"
#include "parser/schedule/factory.h"
#include <sstream>
#include <stack>
#include <vector>

enum Location
{
  START,
  BUNDLE,
  TOPOLOGY,
  NODEGROUPS,
  LINKGROUPS,
  INTERFACEGROUPS,
  NODES,
  LINKS,
  SCHEDULE,
  END
};

struct NodeParseContext
{
  NodeParseContext() :
    module(NULL)
  {}

  NodeParseContext(std::string type) :
    module(NULL),
    type(type)
  {}

  cModule *module;
  std::string type;
  std::string controller;
  std::vector<std::string> control_interfaces;
  std::vector<std::string> data_interfaces;
};

struct ScheduleParseContext
{
  ScheduleParseContext(std::string name, Attr attr) :
    name(name),
    attr(attr)
  {}

  std::string name;
  Attr attr;
  Factory::ChildFactories childFactories;
};

struct ParseContext
{
  ParseContext(Context *context, cModule *parent) :
    abort(false),
    location(START),
    parent(parent),
    context(context)
  {}

  cModule *parent;
  bool abort;
  Location location;
  std::vector<std::string> parents;
  AttrMap ng_attr;
  AttrMap ig_attr;
  AttrMap lg_attr;
  NodeParseContext nPC;
  std::stack<ScheduleParseContext> sPC;
  Context *context;
};

#define FAIL()\
{\
  std::cerr << "Failed at " << __FILE__ << ":" << __LINE__ << std::endl;\
  pC->abort = true;\
  return;\
}

#define CHECK(x) if (!(x))\
{\
  FAIL();\
}

#define UPDATE_LOCATION(x)\
{\
  CHECK(pC->location + 1 == x);\
  pC->location = x;\
}

bool mergeGroups(const std::string &name, Attr &attr,
  const AttrMap &attrMap, GroupMap &groupMap)
{
  auto groups_it = attr.find("groups");
  if (groups_it != attr.end())
  {
    std::istringstream iss(std::move(groups_it->second));
    attr.erase(groups_it);
    std::vector<std::string> groups{
      std::istream_iterator<std::string>(iss), {}};
    for (auto group : groups)
    {
      auto attrMap_it = attrMap.find(group);
      if (attrMap_it == attrMap.end()) return false;
      attr.insert(attrMap_it->second.begin(), attrMap_it->second.end());
      auto groupMap_it = groupMap.find(group);
      if (groupMap_it == groupMap.end()) return false;
      groupMap_it->second.push_back(name);
    }
  }
  return true;
}

static void XMLCALL
startElement(void *userData, const char *csname, const char **atts)
{
  ParseContext *pC = (ParseContext *)userData;
  if (pC->abort) return;
  std::string name(csname);

  Attr attr;
  for (int i = 0; atts[i]; i += 2)
    CHECK(attr.emplace(atts[i], atts[i+1]).second);

  if (pC->parents.empty() && name == "bundle")
  {
    UPDATE_LOCATION(BUNDLE);
  }
  else if (pC->parents.back() == "bundle" && name == "topology")
  {
    UPDATE_LOCATION(TOPOLOGY);
  }
  else if (pC->parents.back() == "topology" && name == "nodegroups")
  {
    UPDATE_LOCATION(NODEGROUPS);
  }
  else if (pC->parents.back() == "topology" && name == "linkgroups")
  {
    UPDATE_LOCATION(LINKGROUPS);
  }
  else if (pC->parents.back() == "topology" && name == "interfacegroups")
  {
    UPDATE_LOCATION(INTERFACEGROUPS);
  }
  else if (name == "group")
  {
    auto group_name_it = attr.find("name");
    CHECK(group_name_it != attr.end());
    std::string group_name(std::move(group_name_it->second));
    attr.erase(group_name_it);
    CHECK(attr.find("groups") == attr.end());
    AttrMap *attrMap;
    GroupMap *groupMap;
    if (pC->parents.back() == "nodegroups")
    {
      attrMap = &pC->ng_attr;
      groupMap = &pC->context->ng;
    }
    else if (pC->parents.back() == "interfacegroups")
    {
      attrMap = &pC->ig_attr;
      groupMap = &pC->context->ig;
    }
    else if (pC->parents.back() == "linkgroups")
    {
      attrMap = &pC->lg_attr;
      groupMap = &pC->context->lg;
    }
    else FAIL();
    CHECK(attrMap->emplace(group_name, std::move(attr)).second);
    CHECK(groupMap->emplace(group_name, std::vector<std::string>()).second)
  }
  else if (pC->parents.back() == "topology" && name == "nodes")
  {
    UPDATE_LOCATION(NODES);
  }
  else if (pC->parents.back() == "nodes" && name == "node")
  {
    auto node_name_it = attr.find("name");
    CHECK(node_name_it != attr.end());
    std::string node_name(std::move(node_name_it->second));
    attr.erase(node_name_it);

    CHECK(mergeGroups(node_name, attr, pC->ng_attr, pC->context->ng));

    auto node_type_it = attr.find("type");
    CHECK(node_type_it != attr.end());
    std::string node_type(node_type_it->second);

    std::string node_name_unique("node::" + node_type + "::" + node_name);
    NodeParseContext nPC(node_type);

    if (node_type == "host")
    {
      cModuleType *hostType = cModuleType::get("inet.nodes.inet.StandardHost");
      nPC.module = hostType->create(node_name_unique.c_str(), pC->parent);
    }
    else if (node_type == "genericSwitch")
    {
      cModuleType *switchType = cModuleType::get(
        "inet.nodes.ethernet.EtherSwitch");
      nPC.module = switchType->create(node_name_unique.c_str(),
        pC->parent);
    }
    else if (node_type == "ofSwitch")
    {
      cModuleType *switchType = cModuleType::get(
        "openflow.openflow.switch.Open_Flow_Switch");
      nPC.module = switchType->create(node_name_unique.c_str(),
        pC->parent);

      auto node_controller_it = attr.find("controller");
      CHECK(node_controller_it != attr.end());
      nPC.controller = node_controller_it->second;
    }
    else if (node_type == "ofController")
    {
      cModuleType *controllerType = cModuleType::get(
        "openflow.openflow.controller.Open_Flow_Controller");
      nPC.module = controllerType->create(
        node_name_unique.c_str(), pC->parent);
    }
    else
    {
      std::cerr << "Unknown node type: " << node_type << std::endl;
      FAIL();
    }

    pC->nPC = nPC;
    pC->context->n.emplace(std::move(node_name), pC->nPC.module);
  }
  else if (pC->parents.back() == "node" && name == "interface")
  {
    auto itfc_name_it = attr.find("name");
    CHECK(itfc_name_it != attr.end());
    std::string itfc_name(itfc_name_it->second);
    attr.erase(itfc_name_it);

    CHECK(mergeGroups(itfc_name, attr, pC->ig_attr, pC->context->ig));

    auto control_it = attr.find("type");
    if (control_it != attr.end() && control_it->second == "control")
    {
      CHECK(pC->nPC.type == "ofSwitch");
      pC->nPC.control_interfaces.push_back(itfc_name);
    }
    else
    {
      pC->nPC.data_interfaces.push_back(itfc_name);
    }
  }
  else if (pC->parents.back() == "topology" && name == "links")
  {
    UPDATE_LOCATION(LINKS);
  }
  else if (pC->parents.back() == "links" && name == "link")
  {
    auto link_name_it = attr.find("name");
    CHECK(link_name_it != attr.end());
    std::string link_name(link_name_it->second);
    attr.erase(link_name_it);

    CHECK(mergeGroups(link_name, attr, pC->lg_attr, pC->context->lg));

    auto a_itfc_name_it = attr.find("a");
    CHECK(a_itfc_name_it != attr.end());
    auto a_itfc_it = pC->context->i.find(a_itfc_name_it->second);
    CHECK(a_itfc_it != pC->context->i.end());

    auto b_itfc_name_it = attr.find("b");
    CHECK(b_itfc_name_it != attr.end());
    auto b_itfc_it = pC->context->i.find(b_itfc_name_it->second);
    CHECK(b_itfc_it != pC->context->i.end());

    auto delay_it = attr.find("delay");
    CHECK(delay_it != attr.end());
    double delay;
    CHECK(parseTime(delay_it->second, delay));

    auto bandwidth_it = attr.find("bandwidth");
    CHECK(bandwidth_it != attr.end());
    double bandwidth;
    CHECK(parseBandwidth(bandwidth_it->second, bandwidth));

    auto errorrate_it = attr.find("error-rate");
    double errorrate;
    if (errorrate_it == attr.end()) errorrate = 0.0;
    else CHECK(parseNumber(errorrate_it->second, errorrate));
    errorrate /= 100;

    std::string a_link_name(std::string("link::a::") + link_name);
    cDatarateChannel *a_cDC = cDatarateChannel::create(a_link_name.c_str());
    a_cDC->setDelay(delay);
    a_cDC->setDatarate(bandwidth);
    a_cDC->setPacketErrorRate(errorrate);
    a_itfc_it->second.first->connectTo(b_itfc_it->second.second, a_cDC);

    std::string b_link_name(std::string("link::b::") + link_name);
    cDatarateChannel *b_cDC = cDatarateChannel::create(b_link_name.c_str());
    b_cDC->setDelay(delay);
    b_cDC->setDatarate(bandwidth);
    b_cDC->setPacketErrorRate(errorrate);
    b_itfc_it->second.first->connectTo(a_itfc_it->second.second, b_cDC);

    CHECK(pC->context->l.emplace(link_name,
                                 std::make_pair(a_cDC, b_cDC)).second);
  }
  else if (pC->parents.back() == "bundle" && name == "schedule")
  {
    UPDATE_LOCATION(SCHEDULE);
    pC->sPC.push(ScheduleParseContext(name, std::move(attr)));
  }
  else if (pC->location == SCHEDULE)
  {
    pC->sPC.push(ScheduleParseContext(name, std::move(attr)));
  }
  else
  {
    std::cerr << "Failed to parse element: " << name << std::endl;
    FAIL();
  }

  pC->parents.push_back(std::move(name));
}

static void XMLCALL
endElement(void *userData, const char *csname)
{
  ParseContext *pC = (ParseContext *)userData;
  if (pC->abort) return;
  std::string name(csname);
  CHECK(name == pC->parents.back())
  pC->parents.pop_back();

  if (pC->location == SCHEDULE)
  {
    ScheduleParseContext sPC = std::move(pC->sPC.top());
    pC->sPC.pop();

    Factory* factory = Factory::parse(
      sPC.name, pC->context, sPC.attr, sPC.childFactories);
    CHECK(factory != NULL);
    for (auto elem : sPC.childFactories)
      CHECK(elem.second.empty());

    if (pC->sPC.empty())
    {
      CHECK(sPC.attr.empty());
      pC->context->factory = factory;
      UPDATE_LOCATION(END);
    }
    else
    {
      pC->sPC.top().childFactories.push_back(
        std::make_pair(factory, sPC.attr));
    }
  }
  else if (name == "node")
  {
    size_t data_gates = pC->nPC.data_interfaces.size();
    size_t ctrl_gates = pC->nPC.control_interfaces.size();

    std::string data_name;
    std::string ctrl_name;

    if (pC->nPC.type == "host")
    {
      data_name = "ethg";
    }
    else if (pC->nPC.type == "genericSwitch")
    {
      data_name = "ethg";
    }
    else if (pC->nPC.type == "ofSwitch")
    {
      data_name = "gateDataPlane";
      ctrl_name = "gateControlPlane";
    }
    else if (pC->nPC.type == "ofController")
    {
      data_name = "ethg";
    }

    if (!data_name.empty())
      pC->nPC.module->setGateSize(data_name.c_str(), data_gates);
    if (!ctrl_name.empty())
      pC->nPC.module->setGateSize(ctrl_name.c_str(), ctrl_gates);

    pC->nPC.module->finalizeParameters();

    for (size_t i = 0; i < data_gates; i++)
    {
      std::string itfc_name = pC->nPC.data_interfaces[i];
      cGate *a = pC->nPC.module->gateHalf(data_name.c_str(), cGate::OUTPUT, i);
      cGate *b = pC->nPC.module->gateHalf(data_name.c_str(), cGate::INPUT, i);
      auto val = std::make_pair(a, b);
      CHECK(pC->context->i.emplace(itfc_name, val).second);
    }

    for (size_t i = 0; i < ctrl_gates; i++)
    {
      std::string itfc_name = pC->nPC.control_interfaces[i];
      cGate *a = pC->nPC.module->gateHalf(ctrl_name.c_str(), cGate::OUTPUT, i);
      cGate *b = pC->nPC.module->gateHalf(ctrl_name.c_str(), cGate::INPUT, i);
      auto val = std::make_pair(a, b);
      CHECK(pC->context->i.emplace(itfc_name, val).second);
    }

    pC->nPC.module->buildInside();

    if (pC->nPC.type == "ofSwitch")
    {
      pC->nPC.module->getSubmodule("OF_Switch")->par("connectAddress") =
        "Seed.seedSimple.node::ofController::" + pC->nPC.controller;
    }
  }
}

bool Parser::parse
(
  const std::string &bundle_path,
  Context &context,
  cModule *parent
)
{
  std::ifstream bundle_file(bundle_path);
  if (!bundle_file.is_open()) return false;

  XML_Parser parser = XML_ParserCreate(NULL);
  if (!parser) return false;

  ParseContext parseContext(&context, parent);

  XML_SetUserData(parser, &parseContext);
  XML_SetElementHandler(parser, startElement, endElement);

  const size_t bufsize = 8192;

  bool done = false;
  char buffer[bufsize];
  while(!done)
  {
    bundle_file.read(buffer, bufsize);
    size_t len = bundle_file.gcount();
    done = len < bufsize;
    if (XML_Parse(parser, buffer, len, done) == XML_STATUS_ERROR)
    {
      std::cerr << "Failed to parse file" << std::endl;
      return false;
    }
  }
  XML_ParserFree(parser);

  if (parseContext.abort || parseContext.location != END) return false;

  bool notFinished = true;
  for (int i = 0; notFinished; ++i) {
    notFinished = false;
    for (auto &elem : context.n)
      notFinished |= elem.second->callInitialize(i);
  }

  for (auto &elem : context.n)
    elem.second->scheduleStart(simTime());

  return true;
}
