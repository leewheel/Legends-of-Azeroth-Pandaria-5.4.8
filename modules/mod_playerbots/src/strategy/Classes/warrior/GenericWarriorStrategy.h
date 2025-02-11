/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_GENERICWARRIORSTRATEGY_H
#define _PLAYERBOT_GENERICWARRIORSTRATEGY_H

#include "CombatStrategy.h"

class PlayerbotAI;

// Stance requirements
class WarriorStanceRequirementActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    WarriorStanceRequirementActionNodeFactory()
    {
        // battle only
        creators["charge"] = &charge;
    }

private:
    ACTION_NODE_P(charge, "charge", "battle stance");
};

class GenericWarriorStrategy : public CombatStrategy
{
public:
    GenericWarriorStrategy(PlayerbotAI* botAI);

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    std::string const getName() override { return "warrior"; }
};

class WarrirorAoeStrategy : public CombatStrategy
{
public:
    WarrirorAoeStrategy(PlayerbotAI* botAI);

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    std::string const getName() override { return "aoe"; }
};

#endif
