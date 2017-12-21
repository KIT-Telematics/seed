#ifndef SEED_SCHEDULE_ON_OFF_EVENT_H
#define SEED_SCHEDULE_ON_OFF_EVENT_H

#include "context.h"
#include <omnetpp.h>
#include "parser/schedule/factory.h"
#include <vector>

struct OnOffEventContext
{
  const Context *context;
  std::function<double(double)> start;
  std::function<std::string(const Context *, double)> source;
  std::function<std::string(const Context *, double)> destination;
  std::function<double(double)> onTime;
  std::function<double(double)> offTime;
  std::function<double(double)> dataRate;
  std::function<double(double)> maxSize;
  std::function<double(double)> packetSize;
};

class OnOffEvent : public cSimpleModule
{
public:
  void postConstruct(std::shared_ptr<OnOffEventContext> onOffEventContext);

protected:
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
  virtual void finish() override;

private:
  std::unique_ptr<cMessage> event;
  std::shared_ptr<OnOffEventContext> onOffEventContext;
};

#endif
