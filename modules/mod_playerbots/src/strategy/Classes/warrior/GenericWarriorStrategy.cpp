#include "GenericWarriorStrategy.h"

#include "Playerbots.h"

class GenericWarriorStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    GenericWarriorStrategyActionNodeFactory()
    {
        creators["heroic strike"] = &GenericWarriorStrategyActionNodeFactory::heroic_strike;
        creators["heroic leap"] = &GenericWarriorStrategyActionNodeFactory::heroic_leap;
        creators["charge"] = &GenericWarriorStrategyActionNodeFactory::charge;

        creators["whirlwind"] = &whirlwind;
    }

private:
    ACTION_NODE_A(heroic_strike, "heroic strike", "melee");
    ACTION_NODE_A(heroic_leap, "heroic leap", "charge");
    ACTION_NODE_A(charge, "charge", "reach melee");

    ACTION_NODE_A(whirlwind, "whirlwind", "melee");
};

GenericWarriorStrategy::GenericWarriorStrategy(PlayerbotAI* botAI) : CombatStrategy(botAI)
{
    actionNodeFactories.Add(new GenericWarriorStrategyActionNodeFactory());
}

void GenericWarriorStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    CombatStrategy::InitTriggers(triggers);
    triggers.push_back(new TriggerNode("enemy out of melee", NextAction::array(0, new NextAction("heroic leap", ACTION_MOVE + 10), nullptr)));
}

class WarriorAoeStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    WarriorAoeStrategyActionNodeFactory()
    {
    }

private:
};

WarriorAoeStrategy::WarriorAoeStrategy(PlayerbotAI* botAI) : CombatStrategy(botAI)
{
    actionNodeFactories.Add(new WarriorAoeStrategyActionNodeFactory());
}

void WarriorAoeStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
}