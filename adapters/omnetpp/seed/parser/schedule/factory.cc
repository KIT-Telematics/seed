#include "parser/schedule/factory.h"

#include "distribution.h"

int Factory::uniqueIndex = 0;

Factory* Factory::parse
(
  const std::string &name,
  const Context *context,
  Attr &attr,
  ChildFactories &childFactories
)
{
  auto factory_it = factories.find(name);
  if (factory_it == factories.end())
    return NULL;
  else
    return factory_it->second(context, attr, childFactories);
}

std::string Factory::newUniqueModuleName(std::string type)
{
  std::stringstream ss;
  ss << "seed.schedule." << type << "." << uniqueIndex++;
  return ss.str();
}

bool Factory::parseStart(Attr &attr, std::function<double(double)> &start)
{
  auto start_it = attr.find("start");
  if (start_it != attr.end())
  {
    if (!parseDistribtion(start_it->second, "s", start))
        return false;
    attr.erase(start_it);
  } else {
    start = [](double rand) {
      return 0.0;
    };
  }
  return true;
}

bool Factory::parseEndpoint
(
  const Context *context,
  const std::string &attr_name,
  Attr &attr,
  std::function<std::string(const Context *, double)> &res
)
{
  auto it = attr.find(attr_name);
  if (it == attr.end()) return false;
  Term term;
  std::string name;
  std::vector<std::string> args;
  if (!parseTerm(it->second, term, name, args)) return false;
  attr.erase(it);

  if (term == VARIABLE)
  {
    if (context->n.find(name) == context->n.end()) return false;
    res = [name](const Context * context, double rand) -> std::string {
      return std::string(name);
    };
  }
  else if (term == FUNCTION)
  {
    if (name != "sample") return false;
    if (args.size() != 1) return false;
    if (context->ng.find(args[0]) == context->ng.end()) return false;
    res = [args](const Context * context, double rand) -> std::string {
      auto it = context->ng.find(args[0]);
      return it->second[(size_t)it->second.size()*rand];
    };
  } else return false;
  return true;
}


static int counter;
static typename std::aligned_storage<
  sizeof (FactoryRegistry), alignof (FactoryRegistry)>::type buffer;
FactoryRegistry &factories = reinterpret_cast<FactoryRegistry&> (buffer);

FactoryRegistrar::FactoryRegistrar ()
{
  if (counter++ == 0) new (&factories) FactoryRegistry ();
}
FactoryRegistrar::FactoryRegistrar(std::string name, Factory::ParserMethod parserMethod)
{
  if (counter++ == 0) new (&factories) FactoryRegistry ();
  factories.emplace(name, parserMethod);
}
FactoryRegistrar::~FactoryRegistrar()
{
  if (--counter == 0) factories.~unordered_map();
}

static FactoryRegistrar factoryRegistrar;
