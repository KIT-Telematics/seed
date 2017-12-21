#include "schedule/process.h"

Define_Module(Process);

void Process::postConstruct(std::shared_ptr<ProcessContext> processContext)
{
  this->processContext = processContext;
  repeat = (int64) processContext->max_repeat(dblrand());
}

void Process::initialize()
{
  event = std::unique_ptr<cMessage>(new cMessage("fire"));
  scheduleAt(simTime() + processContext->start(dblrand()), event.get());
}

void Process::handleMessage(cMessage *msg)
{
  if (msg == event.get() && repeat-- != 0)
  {
    for (Factory* factory : processContext->factories)
    {
      factory->instantiate(this);
    }

    scheduleAt(simTime() + processContext->fire(dblrand()), event.get());
  }
}

void Process::finish()
{
  if (event->isScheduled())
    cancelEvent(event.get());
}
