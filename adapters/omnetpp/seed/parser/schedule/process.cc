#include <parser/schedule/factory.h>

#include <limits>
#include "distribution.h"
#include "schedule/process.h"

class ProcessFactory : public Factory
{
public:
  static Factory* parse
  (
    const Context *context,
    Attr &attr,
    ChildFactories &childFactories
  )
  {
    auto processContext = std::make_shared<ProcessContext>();

    if (!parseStart(attr, processContext->start)) return NULL;

    auto fire_it = attr.find("fire");
    if (fire_it == attr.end()) return NULL;
    std::string sfire(fire_it->second);
    if (!parseDistribtion(fire_it->second, "s", processContext->fire))
        return NULL;
    attr.erase(fire_it);

    auto mrpt_it = attr.find("max-repeat");
    if (mrpt_it == attr.end())
      processContext->max_repeat = [](double rand) {
        return std::numeric_limits<int64>::max();
      };
    else if (parseDistribtion(mrpt_it->second, "", processContext->max_repeat))
      attr.erase(mrpt_it);
    else
      return NULL;

    for (auto childFactory : childFactories)
      processContext->factories.push_back(childFactory.first);

    return new ProcessFactory(processContext);
  }

  cModule* instantiate(cModule* self)
  {
    cModuleType *processT = cModuleType::get("seed.schedule.Process");
    auto name = newUniqueModuleName("Process");
    Process* process = dynamic_cast<Process*>(
      processT->create(name.c_str(), self));
    process->finalizeParameters();
    process->buildInside();
    process->postConstruct(processContext);
    process->callInitialize();
    process->scheduleStart(simTime());
    return process;
  }

private:
  std::shared_ptr<ProcessContext> processContext;

  ProcessFactory (std::shared_ptr<ProcessContext> processContext) :
    processContext(processContext)
  {}
};

REGISTER_FACTORY(process, ProcessFactory);
