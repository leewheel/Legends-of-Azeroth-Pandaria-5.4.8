/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#include "WarriorAiObjectContext.h"

#include "ArmsWarriorStrategy.h"
#include "FuryWarriorStrategy.h"
#include "GenericWarriorNonCombatStrategy.h"
#include "NamedObjectContext.h"
#include "Playerbots.h"
#include "TankWarriorStrategy.h"
#include "WarriorActions.h"
#include "WarriorTriggers.h"

class WarriorStrategyFactoryInternal : public NamedObjectContext<Strategy>
{
public:
    WarriorStrategyFactoryInternal()
    {
        creators["nc"] = &WarriorStrategyFactoryInternal::nc;
    }

private:
    static Strategy* nc(PlayerbotAI* botAI) { return new GenericWarriorNonCombatStrategy(botAI); }
};

class WarriorCombatStrategyFactoryInternal : public NamedObjectContext<Strategy>
{
public:
    WarriorCombatStrategyFactoryInternal() : NamedObjectContext<Strategy>(false, true)
    {
        creators["tank"] = &WarriorCombatStrategyFactoryInternal::tank;
        creators["arms"] = &WarriorCombatStrategyFactoryInternal::arms;
        creators["fury"] = &WarriorCombatStrategyFactoryInternal::fury;
    }
private:
    static Strategy* tank(PlayerbotAI* botAI) { return new TankWarriorStrategy(botAI); }
    static Strategy* arms(PlayerbotAI* botAI) { return new ArmsWarriorStrategy(botAI); }
    static Strategy* fury(PlayerbotAI* botAI) { return new FuryWarriorStrategy(botAI); }
};

class WarriorTriggerFactoryInternal : public NamedObjectContext<Trigger>
{
public:
    WarriorTriggerFactoryInternal()
    {
    }
private:
};

class WarriorAiObjectContextInternal : public NamedObjectContext<Action>
{
public:
    WarriorAiObjectContextInternal()
    {
        creators["charge"] = &WarriorAiObjectContextInternal::charge;
    }
private:
    static Action* charge(PlayerbotAI* botAI) { return new CastChargeAction(botAI); }
};

WarriorAiObjectContext::WarriorAiObjectContext(PlayerbotAI* botAI) : AiObjectContext(botAI)
{
    strategyContexts.Add(new WarriorStrategyFactoryInternal());
    strategyContexts.Add(new WarriorCombatStrategyFactoryInternal());
    actionContexts.Add(new WarriorAiObjectContextInternal());
    triggerContexts.Add(new WarriorTriggerFactoryInternal());
}
