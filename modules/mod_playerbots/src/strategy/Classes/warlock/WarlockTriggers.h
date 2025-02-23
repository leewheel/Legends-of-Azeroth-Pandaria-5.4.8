/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_WARLOCKTRIGGERS_H
#define _PLAYERBOT_WARLOCKTRIGGERS_H

#include "GenericTriggers.h"
#include "PlayerbotAI.h"

class PlayerbotAI;

BUFF_TRIGGER_TIMED(DarkIntentTrigger, "dark intent", 2 * 2000);
class CurseOfAgonyTrigger : public DebuffTrigger
{
public:
    CurseOfAgonyTrigger(PlayerbotAI* botAI) : DebuffTrigger(botAI, "agony", 1, true, 20.0f) {}
};

class CorruptionTrigger : public DebuffTrigger
{
public:
    CorruptionTrigger(PlayerbotAI* botAI) : DebuffTrigger(botAI, "corruption", 1, true) {}
    bool IsActive() override
    {
        return DebuffTrigger::IsActive() && !botAI->HasAura("seed of corruption", GetTarget(), false, true);
    }
};
DEBUFF_CHECKISOWNER_TRIGGER(UnstableAfflictionTrigger, "unstable affliction");

#endif
