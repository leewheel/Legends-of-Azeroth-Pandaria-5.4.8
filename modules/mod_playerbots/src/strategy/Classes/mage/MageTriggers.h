/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_MAGETRIGGERS_H
#define _PLAYERBOT_MAGETRIGGERS_H

#include "GenericTriggers.h"
#include "SharedDefines.h"

class ArcaneBrillianceTrigger : public BuffTrigger
{
public:
    ArcaneBrillianceTrigger(PlayerbotAI* botAI) : BuffTrigger(botAI, "arcane brilliance", 2 * 2000) {}
    bool IsActive() override;
};

class MageArmorTrigger : public BuffTrigger
{
public:
    MageArmorTrigger(PlayerbotAI* botAI) : BuffTrigger(botAI, "mage armor", 5 * 2000) {}
    bool IsActive() override;
};

class LivingBombTrigger : public DebuffTrigger
{
public:
    LivingBombTrigger(PlayerbotAI* botAI) : DebuffTrigger(botAI, "living bomb", 1, true) {}
};

class MissileBarrageTrigger : public HasAuraTrigger
{
public:
    MissileBarrageTrigger(PlayerbotAI* botAI) : HasAuraTrigger(botAI, "missile barrage") {}
};

class ArcaneBlastTrigger : public BuffTrigger
{
public:
    ArcaneBlastTrigger(PlayerbotAI* botAI) : BuffTrigger(botAI, "arcane blast") {}
};

class FingersOfFrostSingleTrigger : public HasAuraStackTrigger
{
public:
    FingersOfFrostSingleTrigger(PlayerbotAI* ai) : HasAuraStackTrigger(ai, "fingers of frost", 1, 1) {}
    bool IsActive() override;
};

class FingersOfFrostDoubleTrigger : public HasAuraStackTrigger
{
public:
    FingersOfFrostDoubleTrigger(PlayerbotAI* ai) : HasAuraStackTrigger(ai, "fingers of frost", 2, 1) {}
};

class ArcanePowerTrigger : public BuffTrigger
{
public:
    ArcanePowerTrigger(PlayerbotAI* botAI) : BuffTrigger(botAI, "arcane power") {}
};

class PresenceOfMindTrigger : public BuffTrigger
{
public:
    PresenceOfMindTrigger(PlayerbotAI* botAI) : BuffTrigger(botAI, "presence of mind") {}
};

#endif
