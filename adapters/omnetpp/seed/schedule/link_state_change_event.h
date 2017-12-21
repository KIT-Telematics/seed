#ifndef SEED_SCHEDULE_LINK_STATE_CHANGE_EVENT_H
#define SEED_SCHEDULE_LINK_STATE_CHANGE_EVENT_H

#include "context.h"
#include <omnetpp.h>
#include "parser/schedule/factory.h"
#include <vector>

struct LSCEContext
{
  const Context *context;
  std::function<double(double)> start;
  std::function<std::string(const Context *, float)> link;
  bool disabled;
};

class LinkStateChangeEvent : public cSimpleModule
{
public:
  void postConstruct(std::shared_ptr<LSCEContext> lSCEContext);

protected:
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
  virtual void finish() override;

private:
  std::unique_ptr<cMessage> event;
  std::shared_ptr<LSCEContext> lSCEContext;
};

#endif
