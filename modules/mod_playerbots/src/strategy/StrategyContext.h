/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_STRATEGYCONTEXT_H
#define _PLAYERBOT_STRATEGYCONTEXT_H


#include "CustomStrategy.h"

#include "NamedObjectContext.h"

#include "ConserveManaStrategy.h"
#include "MeleeCombatStrategy.h"
#include "DeadStrategy.h"
#include "HelloStrategy.h"
#include "RangedCombatStrategy.h"
#include "WorldPacketHandlerStrategy.h"

class StrategyContext : public NamedObjectContext<Strategy>
{
public:
    StrategyContext()
    {
        creators["default"] = &StrategyContext::world_packet;
        creators["custom"] = &StrategyContext::custom;

        creators["save mana"] = &StrategyContext::auto_save_mana;
        creators["close"] = &StrategyContext::close;
        creators["ranged"] = &StrategyContext::ranged;
        creators["dead"] = &StrategyContext::dead;
    }

private:
    
    static Strategy* world_packet(PlayerbotAI* botAI) { return new WorldPacketHandlerStrategy(botAI); }
    static Strategy* custom(PlayerbotAI* botAI) { return new CustomStrategy(botAI); }

    static Strategy* auto_save_mana(PlayerbotAI* botAI) { return new HealerAutoSaveManaStrategy(botAI); }
    static Strategy* close(PlayerbotAI* botAI) { return new MeleeCombatStrategy(botAI); }
    static Strategy* ranged(PlayerbotAI* botAI) { return new RangedCombatStrategy(botAI); }
    static Strategy* dead(PlayerbotAI* botAI) { return new DeadStrategy(botAI); }
};

#endif
