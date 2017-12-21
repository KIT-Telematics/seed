#include <parser/schedule/factory.h>

#include "schedule/schedule.h"

class ScheduleFactory : public Factory
{
public:
  static Factory* parse
  (
    const Context *context,
    Attr &attr,
    ChildFactories &childFactories
  )
  {
    auto scheduleContext = std::make_shared<ScheduleContext>();

    for (auto childFactory : childFactories)
      scheduleContext->factories.push_back(childFactory.first);

    return new ScheduleFactory(scheduleContext);
  }

  cModule* instantiate(cModule* self)
  {
    cModuleType *scheduleT = cModuleType::get("seed.schedule.Schedule");
    auto name = newUniqueModuleName("Schedule");
    Schedule* schedule = dynamic_cast<Schedule*>(
      scheduleT->create(name.c_str(), self));
    schedule->finalizeParameters();
    schedule->buildInside();
    schedule->postConstruct(scheduleContext);
    schedule->callInitialize();
    schedule->scheduleStart(simTime());
    return schedule;
  }
private:
  std::shared_ptr<ScheduleContext> scheduleContext;

  ScheduleFactory (std::shared_ptr<ScheduleContext> scheduleContext) :
    scheduleContext(scheduleContext)
  {}
};

REGISTER_FACTORY(schedule, ScheduleFactory);
