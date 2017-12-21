#ifndef SEED_SCHEDULE_CHOICE_H
#define SEED_SCHEDULE_CHOICE_H

#include <omnetpp.h>
#include "parser/schedule/factory.h"
#include <vector>

struct ChoiceContext
{
  std::function<double(double)> start;
  std::vector<double> probabilities;
  std::vector<Factory*> factories;
};

class Choice : public cSimpleModule
{
public:
  void postConstruct(std::shared_ptr<ChoiceContext> choiceContext);

protected:
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
  virtual void finish() override;

private:
  std::unique_ptr<cMessage> event;
  std::shared_ptr<ChoiceContext> choiceContext;
};

#endif
