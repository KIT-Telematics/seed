#include "schedule/choice.h"

#include <algorithm>

Define_Module(Choice);

void Choice::postConstruct(std::shared_ptr<ChoiceContext> choiceContext)
{
  this->choiceContext = choiceContext;
}

void Choice::initialize()
{
  event = std::unique_ptr<cMessage>(new cMessage("fire"));
  scheduleAt(simTime() + choiceContext->start(dblrand()), event.get());
}

void Choice::handleMessage(cMessage *msg)
{
  if (msg == event.get())
  {
    auto it = std::upper_bound
    (
      choiceContext->probabilities.begin(),
      choiceContext->probabilities.end(),
      dblrand()
    );

    Factory* factory =
      choiceContext->factories[it - choiceContext->probabilities.begin()];
    factory->instantiate(this);
  }
}

void Choice::finish()
{
  if (event->isScheduled())
    cancelEvent(event.get());
}
