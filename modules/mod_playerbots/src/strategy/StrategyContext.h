/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_STRATEGYCONTEXT_H
#define _PLAYERBOT_STRATEGYCONTEXT_H


#include "CustomStrategy.h"

#include "NamedObjectContext.h"

#include "MeleeCombatStrategy.h"
#include "DeadStrategy.h"
#include "HelloStrategy.h"
#include "WorldPacketHandlerStrategy.h"

class StrategyContext : public NamedObjectContext<Strategy>
{
public:
    StrategyContext()
    {
        creators["default"] = &StrategyContext::world_packet;
        creators["custom"] = &StrategyContext::custom;

        creators["close"] = &StrategyContext::close;
        creators["dead"] = &StrategyContext::dead;
    }

private:
    
    static Strategy* world_packet(PlayerbotAI* botAI) { return new WorldPacketHandlerStrategy(botAI); }
    static Strategy* custom(PlayerbotAI* botAI) { return new CustomStrategy(botAI); }

    static Strategy* close(PlayerbotAI* botAI) { return new MeleeCombatStrategy(botAI); }
    static Strategy* dead(PlayerbotAI* botAI) { return new DeadStrategy(botAI); }
};

#endif
