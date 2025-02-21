/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_WARRIORACTIONS_H
#define _PLAYERBOT_WARRIORACTIONS_H

#include "AiObject.h"
#include "GenericSpellActions.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "ReachTargetActions.h"

// -- Generic
REACH_ACTION(CastChargeAction, "charge", 8.0f);
MELEE_ACTION(CastHeroicStrikeAction, "heroic strike");
MELEE_ACTION(CastWhirlWindAction, "whirlwind");

BUFF_ACTION(CastCommandingShoutAction, "commanding shout");
BUFF_ACTION(CastBattleShoutAction, "battle shout");

class CastHeroicLeapAction : public CastSpellAction
{
public:
    CastHeroicLeapAction(PlayerbotAI* botAI) : CastSpellAction(botAI, "heroic leap") {}
    ActionThreatType getThreatType() override { return ActionThreatType::Aoe; }
};

// -- arms
MELEE_ACTION(CastMortalStrikeAction, "mortal strike");
MELEE_ACTION(CastSlamAction, "slam");
MELEE_ACTION(CastOverPowerAction, "overpower");
MELEE_ACTION(CastColossusSmashAction, "colossus smash");

class CastDieByTheSwordAction : public CastBuffSpellAction
{
public:
    CastDieByTheSwordAction(PlayerbotAI* botAI) : CastBuffSpellAction(botAI, "die by the sword") {}
};
class CastSweepingStrikesAction : public CastBuffSpellAction
{
public:
    CastSweepingStrikesAction(PlayerbotAI* botAI) : CastBuffSpellAction(botAI, "sweeping strikes") {}
};

#endif
