#include <parser/schedule/factory.h>

#include "schedule/link_state_change_event.h"

class LSCEFactory : public Factory
{
public:
  static bool parseLink
  (
    const Context *context,
    const std::string &attr_name,
    Attr &attr,
    std::function<std::string(const Context *, float)> &res
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
      if (context->l.find(name) == context->l.end()) return false;
      res = [name](const Context * context, float rand) -> std::string {
        return std::string(name);
      };
    }
    else if (term == FUNCTION)
    {
      if (name != "sample") return false;
      if (args.size() != 1) return false;
      if (context->lg.find(args[0]) == context->lg.end()) return false;
      res = [args](const Context * context, float rand) -> std::string {
        auto it = context->lg.find(args[0]);
        return it->second[(size_t)it->second.size()*rand];
      };
    } else return false;
    return true;
  }

  static Factory* parse
  (
    const Context *context,
    Attr &attr,
    ChildFactories &childFactories
  )
  {
    auto lSCEContext = std::make_shared<LSCEContext>();
    lSCEContext->context = context;

    if (!childFactories.empty()) return NULL;
    if (!parseStart(attr, lSCEContext->start)) return NULL;
    if (!parseLink(context, "link", attr, lSCEContext->link)) return NULL;
    auto state_it = attr.find("state");
    if (state_it == attr.end()) return NULL;
    if (state_it->second != "on" && state_it->second != "off") return NULL;
    lSCEContext->disabled = state_it->second == "off";
    attr.erase(state_it);

    return new LSCEFactory(lSCEContext);
  }

  cModule* instantiate(cModule* self)
  {
    cModuleType *lSCET = cModuleType::get("seed.schedule.LinkStateChangeEvent");
    auto name = newUniqueModuleName("LinkStateChangeEvent");
    LinkStateChangeEvent* lSCE = dynamic_cast<LinkStateChangeEvent*>(
      lSCET->create(name.c_str(), self));
    lSCE->finalizeParameters();
    lSCE->buildInside();
    lSCE->postConstruct(lSCEContext);
    lSCE->callInitialize();
    lSCE->scheduleStart(simTime());
    return lSCE;
  }
private:
  std::shared_ptr<LSCEContext> lSCEContext;

  LSCEFactory (std::shared_ptr<LSCEContext> lSCEContext) :
    lSCEContext(lSCEContext)
  {}
};

REGISTER_FACTORY(link-state-change-event, LSCEFactory);
