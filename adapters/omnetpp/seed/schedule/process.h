#ifndef SEED_SCHEDULE_PROCESS_H
#define SEED_SCHEDULE_PROCESS_H

#include <omnetpp.h>
#include "parser/schedule/factory.h"
#include <vector>

struct ProcessContext
{
  std::function<double(double)> max_repeat;
  std::function<double(double)> start;
  std::function<double(double)> fire;
  std::vector<Factory*> factories;
};

class Process : public cSimpleModule
{
public:
  void postConstruct(std::shared_ptr<ProcessContext> processContext);

protected:
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
  virtual void finish() override;

private:
  std::unique_ptr<cMessage> event;
  std::shared_ptr<ProcessContext> processContext;

  int64 repeat;
};

#endif
