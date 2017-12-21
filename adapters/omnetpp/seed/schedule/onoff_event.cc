#include "schedule/onoff_event.h"

#include "applications/seed_onoff_client.h"
#include "applications/seed_drop_server.h"

Define_Module(OnOffEvent);

void OnOffEvent::postConstruct(std::shared_ptr<OnOffEventContext> onOffEventContext)
{
  this->onOffEventContext = onOffEventContext;
}

void OnOffEvent::initialize()
{
  event = std::unique_ptr<cMessage>(new cMessage("fire"));
  scheduleAt(simTime() + onOffEventContext->start(dblrand()), event.get());
}

void OnOffEvent::handleMessage(cMessage *msg)
{
  if (msg == event.get())
  {
    auto c = onOffEventContext->context;
    auto source = onOffEventContext->source(c, dblrand());
    auto destination = onOffEventContext->destination(c, dblrand());
    auto cC = c->n.find(source)->second;
    auto cSOC = check_and_cast<SEEDOnOffClient *>(
      cC->getSubmodule("tcpApp", 2));
    auto cS = c->n.find(destination)->second;
    auto cSDS = check_and_cast<SEEDDropServer *>(
      cS->getSubmodule("tcpApp", 0));
    auto destPort = cSDS->addListenerOnce();
    auto dataRate = onOffEventContext->dataRate(dblrand());
    auto maxSize = onOffEventContext->maxSize(dblrand());
    auto packetSize = onOffEventContext->packetSize(dblrand());
    cSOC->send(
      cS->getFullPath().c_str(), destPort,
      onOffEventContext->onTime,
      onOffEventContext->offTime,
      dataRate,
      (int64) maxSize,
      (int64) packetSize
    );
  }
}

void OnOffEvent::finish()
{
  if (event->isScheduled())
    cancelEvent(event.get());
}
