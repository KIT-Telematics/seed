#include "schedule/link_state_change_event.h"

Define_Module(LinkStateChangeEvent);

void LinkStateChangeEvent::postConstruct(std::shared_ptr<LSCEContext> lSCEContext)
{
  this->lSCEContext = lSCEContext;
}

void LinkStateChangeEvent::initialize()
{
  event = std::unique_ptr<cMessage>(new cMessage("fire"));
  scheduleAt(simTime() + lSCEContext->start(dblrand()), event.get());
}

void LinkStateChangeEvent::handleMessage(cMessage *msg)
{
  if (msg == event.get())
  {
    auto c = lSCEContext->context;
    auto link = lSCEContext->link(c, dblrand());
    c->n.find(link);
    auto cLink = c->l.find(link)->second;
    cLink.first->setDisabled(lSCEContext->disabled);
    cLink.second->setDisabled(lSCEContext->disabled);
  }
}

void LinkStateChangeEvent::finish()
{
  if (event->isScheduled())
    cancelEvent(event.get());
}
