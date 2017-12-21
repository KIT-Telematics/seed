#include <parser/schedule/factory.h>

#include "schedule/onoff_event.h"
#include "distribution.h"
#include "context.h"

class OnOffEventFactory : public Factory
{
public:
  static Factory* parse
  (
    const Context *context,
    Attr &attr,
    ChildFactories &childFactories
  )
  {
    auto onOffEventContext = std::make_shared<OnOffEventContext>();
    onOffEventContext->context = context;

    if (!childFactories.empty()) return NULL;
    if (!parseStart(attr, onOffEventContext->start)) return NULL;
    if (!parseEndpoint(context, "source", attr,
      onOffEventContext->source)) return NULL;
    if (!parseEndpoint(context, "destination", attr,
      onOffEventContext->destination)) return NULL;
    auto onTime_it = attr.find("on-time");
    if (onTime_it == attr.end()) return NULL;
    if (!parseDistribtion(onTime_it->second, "s", onOffEventContext->onTime))
      return NULL;
    attr.erase(onTime_it);
    auto offTime_it = attr.find("off-time");
    if (offTime_it == attr.end()) return NULL;
    if (!parseDistribtion(offTime_it->second, "s", onOffEventContext->offTime))
      return NULL;
    attr.erase(offTime_it);
    auto dataRate_it = attr.find("data-rate");
    if (dataRate_it == attr.end()) return NULL;
    if (!parseDistribtion(dataRate_it->second, "bps",
      onOffEventContext->dataRate))
      return NULL;
    attr.erase(dataRate_it);
    auto maxSize_it = attr.find("max-size");
    if (maxSize_it == attr.end()) return NULL;
    if (!parseDistribtion(maxSize_it->second, "bit",
      onOffEventContext->maxSize))
      return NULL;
    attr.erase(maxSize_it);
    auto packetSize_it = attr.find("packet-size");
    if (packetSize_it == attr.end()) return NULL;
    if (!parseDistribtion(packetSize_it->second, "bit",
      onOffEventContext->packetSize))
      return NULL;
    attr.erase(packetSize_it);

    return new OnOffEventFactory(onOffEventContext);
  }

  cModule* instantiate(cModule* self)
  {
    cModuleType *onOffEventT = cModuleType::get("seed.schedule.OnOffEvent");
    auto name = Factory::newUniqueModuleName("OnOffEvent");
    OnOffEvent* onOffEvent = dynamic_cast<OnOffEvent*>(
      onOffEventT->create(name.c_str(), self));
    onOffEvent->finalizeParameters();
    onOffEvent->buildInside();
    onOffEvent->postConstruct(onOffEventContext);
    onOffEvent->callInitialize();
    onOffEvent->scheduleStart(simTime());
    return onOffEvent;
  }

private:
  std::shared_ptr<OnOffEventContext> onOffEventContext;

  OnOffEventFactory (std::shared_ptr<OnOffEventContext> onOffEventContext) :
    onOffEventContext(onOffEventContext)
  {}

};

REGISTER_FACTORY(on-off-event, OnOffEventFactory);
