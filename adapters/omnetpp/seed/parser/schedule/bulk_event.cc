#include <parser/schedule/factory.h>

#include "schedule/bulk_event.h"
#include "distribution.h"
#include "context.h"

class BulkEventFactory : public Factory
{
public:
  static Factory* parse
  (
    const Context *context,
    Attr &attr,
    ChildFactories &childFactories
  )
  {
    auto bulkEventContext = std::make_shared<BulkEventContext>();
    bulkEventContext->context = context;

    if (!childFactories.empty()) return NULL;
    if (!parseStart(attr, bulkEventContext->start)) return NULL;
    if (!parseEndpoint(context, "source", attr,
      bulkEventContext->source)) return NULL;
    if (!parseEndpoint(context, "destination", attr,
      bulkEventContext->destination)) return NULL;
    auto size_it = attr.find("max-size");
    if (size_it == attr.end()) return NULL;
    if (!parseDistribtion(size_it->second, "bit", bulkEventContext->size))
        return NULL;
    attr.erase(size_it);


    return new BulkEventFactory(bulkEventContext);
  }

  cModule* instantiate(cModule* self)
  {
    cModuleType *bulkEventT = cModuleType::get("seed.schedule.BulkEvent");
    auto name = Factory::newUniqueModuleName("BulkEvent");
    BulkEvent* bulkEvent = dynamic_cast<BulkEvent*>(
      bulkEventT->create(name.c_str(), self));
    bulkEvent->finalizeParameters();
    bulkEvent->buildInside();
    bulkEvent->postConstruct(bulkEventContext);
    bulkEvent->callInitialize();
    bulkEvent->scheduleStart(simTime());
    return bulkEvent;
  }

private:
  std::shared_ptr<BulkEventContext> bulkEventContext;

  BulkEventFactory (std::shared_ptr<BulkEventContext> bulkEventContext) :
    bulkEventContext(bulkEventContext)
  {}

};

REGISTER_FACTORY(bulk-event, BulkEventFactory);
