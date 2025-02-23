#include "DemonoWarlockStrategy.h"
#include "Playerbots.h"

class DemonoWarlockStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    DemonoWarlockStrategyActionNodeFactory()
    {
    }

private:
};

DemonoWarlockStrategy::DemonoWarlockStrategy(PlayerbotAI* botAI) : GenericWarlockStrategy(botAI)
{
    actionNodeFactories.Add(new DemonoWarlockStrategyActionNodeFactory());
}

NextAction** DemonoWarlockStrategy::getDefaultActions()
{
    return NextAction::array(0,
        nullptr);
}

void DemonoWarlockStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    GenericWarlockStrategy::InitTriggers(triggers);
}

