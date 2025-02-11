/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#include "GenericWarriorStrategy.h"

#include "Playerbots.h"

GenericWarriorStrategy::GenericWarriorStrategy(PlayerbotAI* botAI) : CombatStrategy(botAI)
{
}

void GenericWarriorStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    CombatStrategy::InitTriggers(triggers);
    triggers.push_back(new TriggerNode("enemy out of melee", NextAction::array(0, new NextAction("reach melee", ACTION_HIGH + 1), nullptr)));
}

class WarrirorAoeStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    WarrirorAoeStrategyActionNodeFactory()
    {
    }

private:
};

WarrirorAoeStrategy::WarrirorAoeStrategy(PlayerbotAI* botAI) : CombatStrategy(botAI)
{
    actionNodeFactories.Add(new WarrirorAoeStrategyActionNodeFactory());
}

void WarrirorAoeStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
}
