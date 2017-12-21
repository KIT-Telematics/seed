#ifndef SEED_SCHEDULE_BULK_EVENT_H
#define SEED_SCHEDULE_BULK_EVENT_H

#include "context.h"
#include <omnetpp.h>
#include "parser/schedule/factory.h"
#include <vector>

struct BulkEventContext
{
  const Context *context;
  std::function<double(double)> start;
  std::function<std::string(const Context *, double)> source;
  std::function<std::string(const Context *, double)> destination;
  std::function<double(double)> size;
};

class BulkEvent : public cSimpleModule
{
public:
  void postConstruct(std::shared_ptr<BulkEventContext> bulkEventContext);

protected:
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
  virtual void finish() override;

private:
  std::unique_ptr<cMessage> event;
  std::shared_ptr<BulkEventContext> bulkEventContext;
};

#endif
