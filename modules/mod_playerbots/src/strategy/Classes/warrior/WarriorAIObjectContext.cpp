/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#include "WarriorAIObjectContext.h"

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
        creators["battle shout"] = &WarriorTriggerFactoryInternal::battle_shout;
        creators["commanding shout"] = &WarriorTriggerFactoryInternal::commanding_shout;
    }
private:
    static Trigger* battle_shout(PlayerbotAI* botAI) { return new BattleShoutTrigger(botAI); }
    static Trigger* commanding_shout(PlayerbotAI* botAI) { return new CommandingShoutTrigger(botAI); }
};

class WarriorAiObjectContextInternal : public NamedObjectContext<Action>
{
public:
    WarriorAiObjectContextInternal()
    {
        // -- buffs
        creators["battle shout"] = &WarriorAiObjectContextInternal::battle_shout;
        creators["commanding shout"] = &WarriorAiObjectContextInternal::commanding_shout;

        // -- general
        creators["charge"] = &WarriorAiObjectContextInternal::charge;
        creators["heroic strike"] = &WarriorAiObjectContextInternal::heroic_strike;
        creators["heroic leap"] = &WarriorAiObjectContextInternal::heroic_leap;

        creators["whirlwind"] = &WarriorAiObjectContextInternal::whirlwind;

        // -- arms
        creators["mortal strike"] = &WarriorAiObjectContextInternal::mortal_strike;
        creators["slam"] = &WarriorAiObjectContextInternal::slam;

        creators["overpower"] = &WarriorAiObjectContextInternal::overpower;
        creators["die by the sword"] = &WarriorAiObjectContextInternal::die_by_the_sword;
        creators["sweeping strikes"] = &WarriorAiObjectContextInternal::sweeping_strikes;
        creators["colossus smash"] = &WarriorAiObjectContextInternal::colossus_smash;
    }
private:
    // -- buffs
    static Action* battle_shout(PlayerbotAI* botAI) { return new CastBattleShoutAction(botAI); }
    static Action* commanding_shout(PlayerbotAI* botAI) { return new CastCommandingShoutAction(botAI); }

    // -- generic
    static Action* charge(PlayerbotAI* botAI) { return new CastChargeAction(botAI); }
    static Action* heroic_strike(PlayerbotAI* botAI) { return new CastHeroicStrikeAction(botAI); }
    static Action* heroic_leap(PlayerbotAI* botAI) { return new CastHeroicLeapAction(botAI); }

    static Action* whirlwind(PlayerbotAI* botAI) { return new CastWhirlWindAction(botAI); }

    // -- ARMS
    static Action* mortal_strike(PlayerbotAI* botAI) { return new CastMortalStrikeAction(botAI); }
    static Action* slam(PlayerbotAI* botAI) { return new CastSlamAction(botAI); }
    static Action* overpower(PlayerbotAI* botAI) { return new CastOverPowerAction(botAI); }
    static Action* die_by_the_sword(PlayerbotAI* botAI) { return new CastDieByTheSwordAction(botAI); }
    static Action* sweeping_strikes(PlayerbotAI* botAI) { return new CastSweepingStrikesAction(botAI); }
    static Action* colossus_smash(PlayerbotAI* botAI) { return new CastColossusSmashAction(botAI); }
};

WarriorAiObjectContext::WarriorAiObjectContext(PlayerbotAI* botAI) : AiObjectContext(botAI)
{
    strategyContexts.Add(new WarriorStrategyFactoryInternal());
    strategyContexts.Add(new WarriorCombatStrategyFactoryInternal());
    actionContexts.Add(new WarriorAiObjectContextInternal());
    triggerContexts.Add(new WarriorTriggerFactoryInternal());
}
