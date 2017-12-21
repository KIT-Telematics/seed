#ifndef SEED_PARSER_SCHEDULE_FACTORY_H
#define SEED_PARSER_SCHEDULE_FACTORY_H

#include <memory>
#include <omnetpp.h>
#include "context.h"
#include "parser/internal.h"

struct Context;

class Factory
{
public:
  typedef std::vector<std::pair<Factory*, Attr>> ChildFactories;
  typedef Factory*(*ParserMethod)(const Context *, Attr &, ChildFactories &);

  static Factory* parse
  (
    const std::string &name,
    const Context *context,
    Attr &attr,
    ChildFactories &childFactories
  );

  virtual cModule* instantiate(cModule* self) = 0;

protected:
  static std::string newUniqueModuleName(std::string type);
  static bool parseStart(Attr &attr, std::function<double(double)> &start);
  static bool parseEndpoint
  (
    const Context *context,
    const std::string &attr_name,
    Attr &attr,
    std::function<std::string(const Context *, double)> &res
  );

private:
  static int uniqueIndex;
};

typedef std::unordered_map<std::string, Factory::ParserMethod> FactoryRegistry;
extern FactoryRegistry &factories;

struct FactoryRegistrar {
  FactoryRegistrar ();
  FactoryRegistrar (std::string name, Factory::ParserMethod parserMethod);
  ~FactoryRegistrar ();
};

#define REGISTER_FACTORY(NAME, CLASS)\
static FactoryRegistrar FactoryRegistrar(#NAME, CLASS::parse)

#endif
