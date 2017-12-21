#include "schedule/schedule.h"

Define_Module(Schedule);

void Schedule::postConstruct(std::shared_ptr<ScheduleContext> scheduleContext)
{
  this->scheduleContext = scheduleContext;
}

void Schedule::initialize()
{
  event = std::unique_ptr<cMessage>(new cMessage("fire"));
  scheduleAt(simTime(), event.get());
}

void Schedule::handleMessage(cMessage *msg)
{
  if (msg == event.get())
  {
    for (Factory* factory : scheduleContext->factories)
    {
      factory->instantiate(this);
    }
  }
}

void Schedule::finish()
{
  if (event->isScheduled())
    cancelEvent(event.get());
}
