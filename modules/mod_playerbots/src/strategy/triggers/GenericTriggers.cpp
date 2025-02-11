/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#include "GenericTriggers.h"

#include "Playerbots.h"
#include "PlayerbotAIConfig.h"
#include "Player.h"

bool TimerTrigger::IsActive()
{
    if (time(nullptr) != lastCheck)
    {
        lastCheck = time(nullptr);
        return true;
    }

    return false;
}

RandomTrigger::RandomTrigger(PlayerbotAI* botAI, std::string const name, int32 probability)
    : Trigger(botAI, name), probability(probability), lastCheck(getMSTime())
{
}

bool RandomTrigger::IsActive()
{
    if (getMSTime() - lastCheck < sPlayerbotAIConfig->repeatDelay)
        return false;

    lastCheck = getMSTime();
    int32 k = (int32)(probability / sPlayerbotAIConfig->randomChangeMultiplier);
    if (k < 1)
        k = 1;
    return (rand() % k) == 0;
}

bool CorpseNearTrigger::IsActive()
{
    return bot->GetCorpse() && bot->GetCorpse()->IsWithinDistInMap(bot, CORPSE_RECLAIM_RADIUS, true);
}

bool InvalidTargetTrigger::IsActive()
{
    return AI_VALUE2(bool, "invalid target", "current target");
}

bool NoTargetTrigger::IsActive()
{
    return !AI_VALUE(Unit*, "current target");
}

bool TargetChangedTrigger::IsActive()
{
    Unit* oldTarget = context->GetValue<Unit*>("old target")->Get();
    Unit* target = context->GetValue<Unit*>("current target")->Get();
    return target && oldTarget != target;
}

bool IsNotFacingTargetTrigger::IsActive()
{
    return !AI_VALUE2(bool, "facing", "current target");
}