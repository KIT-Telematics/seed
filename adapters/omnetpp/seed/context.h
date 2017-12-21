#ifndef SEED_CONTEXT_H
#define SEED_CONTEXT_H

#include <omnetpp.h>
#include "parser/schedule/factory.h"
#include <unordered_map>
#include <vector>

class Factory;

typedef std::unordered_map<std::string, std::vector<std::string>> GroupMap;
typedef std::pair<cGate*, cGate*> BiGate;
typedef std::pair<cDatarateChannel*, cDatarateChannel*> BiChannel;

struct Context
{
  GroupMap ng;
  GroupMap ig;
  GroupMap lg;

  std::unordered_map<std::string, cModule*> n;
  std::unordered_map<std::string, BiGate> i;
  std::unordered_map<std::string, BiChannel> l;

  Factory *factory;
};

#endif
