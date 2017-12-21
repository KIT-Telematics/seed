#ifndef SEED_SCHEDULE_SCHEDULE_H
#define SEED_SCHEDULE_SCHEDULE_H

#include <omnetpp.h>
#include "parser/schedule/factory.h"
#include <vector>

struct ScheduleContext
{
  std::vector<Factory*> factories;
};

class Schedule : public cSimpleModule
{
public:
  void postConstruct(std::shared_ptr<ScheduleContext> scheduleContext);

protected:
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
  virtual void finish() override;

private:
  std::unique_ptr<cMessage> event;
  std::shared_ptr<ScheduleContext> scheduleContext;
};

#endif
