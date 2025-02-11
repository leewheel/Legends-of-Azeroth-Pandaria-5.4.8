/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_GENERICTRIGGERS_H
#define _PLAYERBOT_GENERICTRIGGERS_H

#include <utility>

#include "Trigger.h"

class PlayerbotAI;
class Unit;

class RandomTrigger : public Trigger
{
public:
    RandomTrigger(PlayerbotAI* botAI, std::string const name, int32 probability = 7);

    bool IsActive() override;

protected:
    int32 probability;
    uint32 lastCheck;
};

class TimerTrigger : public Trigger
{
public:
    TimerTrigger(PlayerbotAI* botAI) : Trigger(botAI, "timer"), lastCheck(0) {}

    bool IsActive() override;

private:
    time_t lastCheck;
};

class AlwaysTrigger : public Trigger
{
public:
    AlwaysTrigger(PlayerbotAI* ai) : Trigger(ai, "always trigger") {}

    bool IsActive() override
    {
        return true;
    }
};

class CorpseNearTrigger : public Trigger
{
public:
    CorpseNearTrigger(PlayerbotAI* botAI) : Trigger(botAI, "corpse near", 1 * 1000) {}

    bool IsActive() override;
};

class InvalidTargetTrigger : public Trigger
{
public:
    InvalidTargetTrigger(PlayerbotAI* botAI) : Trigger(botAI, "invalid target") {}

    bool IsActive() override;
};

class TargetChangedTrigger : public Trigger
{
public:
    TargetChangedTrigger(PlayerbotAI* botAI) : Trigger(botAI, "target changed") {}

    bool IsActive() override;
};

class NoTargetTrigger : public Trigger
{
public:
    NoTargetTrigger(PlayerbotAI* botAI) : Trigger(botAI, "no target") {}

    bool IsActive() override;
};

class IsNotFacingTargetTrigger : public Trigger
{
public:
    IsNotFacingTargetTrigger(PlayerbotAI* botAI) : Trigger(botAI, "not facing target") {}

    bool IsActive() override;
};

#endif
