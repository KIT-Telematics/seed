#include "schedule/bulk_event.h"

#include "applications/seed_bulk_client.h"
#include "applications/seed_drop_server.h"

Define_Module(BulkEvent);

void BulkEvent::postConstruct(std::shared_ptr<BulkEventContext> bulkEventContext)
{
  this->bulkEventContext = bulkEventContext;
}

void BulkEvent::initialize()
{
  event = std::unique_ptr<cMessage>(new cMessage("fire"));
  scheduleAt(simTime() + bulkEventContext->start(dblrand()), event.get());
}

void BulkEvent::handleMessage(cMessage *msg)
{
  if (msg == event.get())
  {
    auto c = bulkEventContext->context;
    auto source = bulkEventContext->source(c, dblrand());
    auto destination = bulkEventContext->destination(c, dblrand());
    auto cC = c->n.find(source)->second;
    auto cSBC = check_and_cast<SEEDBulkClient *>(
      cC->getSubmodule("tcpApp", 1));
    auto cS = c->n.find(destination)->second;
    auto cSDS = check_and_cast<SEEDDropServer *>(
      cS->getSubmodule("tcpApp", 0));
    auto destPort = cSDS->addListenerOnce();
    auto size = bulkEventContext->size(dblrand());
    cSBC->send(cS->getFullPath().c_str(), destPort, (int64) size);
  }
}

void BulkEvent::finish()
{
  if (event->isScheduled())
    cancelEvent(event.get());
}
