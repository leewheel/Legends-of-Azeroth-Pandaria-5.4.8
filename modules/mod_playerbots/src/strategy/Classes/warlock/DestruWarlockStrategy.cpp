#include "DestruWarlockStrategy.h"
#include "Playerbots.h"

class DestruWarlockStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    DestruWarlockStrategyActionNodeFactory()
    {
    }

private:
};

DestruWarlockStrategy::DestruWarlockStrategy(PlayerbotAI* botAI) : GenericWarlockStrategy(botAI)
{
    actionNodeFactories.Add(new DestruWarlockStrategyActionNodeFactory());
}

NextAction** DestruWarlockStrategy::getDefaultActions()
{
    return NextAction::array(0,
        nullptr);
}

void DestruWarlockStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    GenericWarlockStrategy::InitTriggers(triggers);
}

