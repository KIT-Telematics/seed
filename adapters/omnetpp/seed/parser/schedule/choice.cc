#include <parser/schedule/factory.h>

#include "schedule/choice.h"

class ChoiceFactory : public Factory
{
public:
  static Factory* parse
  (
    const Context *context,
    Attr &attr,
    ChildFactories &childFactories
  )
  {
    auto choiceContext = std::make_shared<ChoiceContext>();

    if (!parseStart(attr, choiceContext->start)) return NULL;

    double weight_sum = 0.0;
    for (auto &childFactory : childFactories)
    {
      auto weight_it = childFactory.second.find("weight");
      if (weight_it == childFactory.second.end()) return NULL;
      double weight;
      if (!parseNumber(weight_it->second, weight)) return NULL;
      childFactory.second.erase(weight_it);
      choiceContext->probabilities.push_back(weight_sum += weight);
      choiceContext->factories.push_back(childFactory.first);
    }

    for (auto &weight : choiceContext->probabilities)
      weight /= weight_sum;

    return new ChoiceFactory(choiceContext);
  }

  cModule* instantiate(cModule* self)
  {
    cModuleType *choiceT = cModuleType::get("seed.schedule.Choice");
    auto name = Factory::newUniqueModuleName("Choice");
    Choice* choice = dynamic_cast<Choice*>(
      choiceT->create(name.c_str(), self));
    choice->finalizeParameters();
    choice->buildInside();
    choice->postConstruct(choiceContext);
    choice->callInitialize();
    choice->scheduleStart(simTime());
    return choice;
  }

private:
  std::shared_ptr<ChoiceContext> choiceContext;

  ChoiceFactory (std::shared_ptr<ChoiceContext> choiceContext) :
    choiceContext(choiceContext)
  {}
};

REGISTER_FACTORY(choice, ChoiceFactory);
