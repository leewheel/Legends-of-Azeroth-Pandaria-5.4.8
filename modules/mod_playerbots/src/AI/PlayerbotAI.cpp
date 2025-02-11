/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#include "PlayerbotAI.h"

#include <cmath>
#include <mutex>
#include <sstream>
#include <string>

#include "AiFactory.h"
#include "Engine.h"
#include "ExternalEventHelper.h"
#include "Playerbots.h"
#include "PlayerbotAIConfig.h"
#include "ChannelMgr.h"
#include "CreatureAIImpl.h"
#include "GuildMgr.h"
#include "Helper.h"
#include "LastMovementValue.h"
#include "LastSpellCastValue.h"
#include "LFGMgr.h"
#include "MapManager.h"
#include "MotionMaster.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "Player.h"
#include "PositionValue.h"
#include "PointMovementGenerator.h"
#include "RandomPlayerbotMgr.h"
#include "ServerFacade.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "SocialMgr.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"
#include "Transport.h"
#include "TradeData.h"
#include "Unit.h"
#include "Vehicle.h"
#include "UpdateFields.h"

void PacketHandlingHelper::AddHandler(uint16 opcode, std::string const handler)
{
    _handlers[opcode] = handler;
}

void PacketHandlingHelper::Handle(ExternalEventHelper& helper)
{
    while (!_queue.empty())
    {
        helper.HandlePacket(_handlers, _queue.top());
        _queue.pop();
    }
}

void PacketHandlingHelper::AddPacket(WorldPacket const& packet)
{
    if (packet.empty())
        return;
    // assert(handlers);
    // assert(packet);
    // assert(packet.GetOpcode());
    if (_handlers.find(packet.GetOpcode()) != _handlers.end())
        _queue.push(WorldPacket(packet));
}

PlayerbotAI::PlayerbotAI()
    : PlayerbotAIBase(true),
    bot(nullptr),
    accountId(0),
    master(nullptr),
    _aiObjectContext{ nullptr },
    _currentEngine{ nullptr },
    _engines{ nullptr },
    _currentState(BOT_STATE_NON_COMBAT)
{
}

PlayerbotAI::PlayerbotAI(Player* bot)
    : PlayerbotAIBase(true),
    bot(bot),
    master(nullptr),
    _aiObjectContext{ nullptr },
    _currentEngine{ nullptr },
    _engines{ nullptr }
{
    accountId = bot->GetSession()->GetAccountId();
    _aiObjectContext = AiFactory::createAiObjectContext(bot, this);
    _engines[BOT_STATE_COMBAT] = AiFactory::createCombatEngine(bot, this, _aiObjectContext);
    _engines[BOT_STATE_NON_COMBAT] = AiFactory::createNonCombatEngine(bot, this, _aiObjectContext);
    _engines[BOT_STATE_DEAD] = AiFactory::createDeadEngine(bot, this, _aiObjectContext);

    _currentEngine = _engines[BOT_STATE_NON_COMBAT];
    _currentState = BOT_STATE_NON_COMBAT;
    _rpgInfo.status = NewRpgStatus::IDLE;

    masterIncomingPacketHandlers.AddHandler(CMSG_GROUP_DISBAND, "uninvite");
    masterIncomingPacketHandlers.AddHandler(CMSG_GROUP_UNINVITE_GUID, "uninvite guid");
    masterIncomingPacketHandlers.AddHandler(CMSG_REPOP_REQUEST, "release spirit");
    masterIncomingPacketHandlers.AddHandler(CMSG_RECLAIM_CORPSE, "revive from corpse");

    botOutgoingPacketHandlers.AddHandler(SMSG_GROUP_INVITE, "group invite");
}

PlayerbotAI::~PlayerbotAI()
{
    for (uint8 i = 0; i < BOT_STATE_MAX; i++)
    {
        if (_engines[i])
            delete _engines[i];
    }

    if (_aiObjectContext)
        delete _aiObjectContext;
}

uint32 PlayerbotAI::GetReactDelay()
{
    uint32 base = sPlayerbotAIConfig->reactDelay;  // Default 100(ms)

    // If dynamic react delay is disabled, use a static calculation
    if (!sPlayerbotAIConfig->dynamicReactDelay)
    {
        if (HasRealPlayerMaster())
            return base;

        bool inBG = bot->InBattleground() || bot->InArena();
        if (/*sPlayerbotAIConfig->fastReactInBG &&*/ inBG)
            return base;

        bool inCombat = bot->IsInCombat();

        if (!inCombat)
            return base * 10.0f;

        else if (inCombat)
            return base * 2.5f;

        return base;
    }

    // Dynamic react delay calculation:

    if (HasRealPlayerMaster())
        return base;

    float multiplier = 1.0f;
    bool inBG = bot->InBattleground() || bot->InArena();

    /*if (inBG)
    {
        if (bot->IsInCombat() || _currentState == BOT_STATE_COMBAT)
        {
            multiplier = sPlayerbotAIConfig->fastReactInBG ? 2.5f : 5.0f;
            return base * multiplier;
        }
        else
        {
            multiplier = sPlayerbotAIConfig->fastReactInBG ? 1.0f : 10.0f;
            return base * multiplier;
        }
    }*/

    // When in combat, return 5 times the base
    if (bot->IsInCombat() || _currentState == BOT_STATE_COMBAT)
    {
        multiplier = 5.0f;
        return base * multiplier;
    }

    // When not resting, return 10-30 times the base
    if (!bot->HasFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_RESTING))
    {
        multiplier = urand(10, 30);
        return base * multiplier;
    }

    // In other cases, return 20-200 times the base
    multiplier = urand(20, 200);
    return base * multiplier;
}

void PlayerbotAI::UpdateAI(uint32 elapsed, bool minimal)
{
    // Handle the AI check delay
    if (nextAICheckDelay > elapsed)
        nextAICheckDelay -= elapsed;
    else
        nextAICheckDelay = 0;

    // Early return if bot is in invalid state
    if (!bot || !bot->IsInWorld() || !bot->GetSession() || bot->GetSession()->isLogingOut() ||
        bot->IsDuringRemoveFromWorld())
        return;

    AllowActivity();

    if (!CanUpdateAI())
        return;

    // Handle the current spell
    Spell* currentSpell = bot->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!currentSpell)
        currentSpell = bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
    if (currentSpell)
    {
        const SpellInfo* spellInfo = currentSpell->GetSpellInfo();
        if (spellInfo && currentSpell->getState() == SPELL_STATE_PREPARING)
        {
            Unit* spellTarget = currentSpell->m_targets.GetUnitTarget();
            // Interrupt if target is dead or spell can't target dead units
            if (spellTarget && !spellTarget->IsAlive() && !spellInfo->IsAllowingDeadTarget())
            {
                InterruptSpell();
                YieldThread(GetReactDelay());
                return;
            }

            bool isHeal = false;
            bool isSingleTarget = true;

            for (uint8 i = 0; i < 3; ++i)
            {
                if (!spellInfo->Effects[i].Effect)
                    continue;

                // Check if spell is a heal
                if (spellInfo->Effects[i].Effect == SPELL_EFFECT_HEAL ||
                    spellInfo->Effects[i].Effect == SPELL_EFFECT_HEAL_MAX_HEALTH ||
                    spellInfo->Effects[i].Effect == SPELL_EFFECT_HEAL_MECHANICAL)
                    isHeal = true;

                // Check if spell is single-target
                if ((spellInfo->Effects[i].TargetA.GetTarget() &&
                    spellInfo->Effects[i].TargetA.GetTarget() != TARGET_UNIT_TARGET_ALLY) ||
                    (spellInfo->Effects[i].TargetB.GetTarget() &&
                        spellInfo->Effects[i].TargetB.GetTarget() != TARGET_UNIT_TARGET_ALLY))
                {
                    isSingleTarget = false;
                }
            }

            // Interrupt if target ally has full health (heal by other member)
            if (isHeal && isSingleTarget && spellTarget && spellTarget->IsFullHealth())
            {
                InterruptSpell();
                YieldThread(GetReactDelay());
                return;
            }

            // Ensure bot is facing target if necessary
            if (spellTarget && !bot->HasInArc(CAST_ANGLE_IN_FRONT, spellTarget) &&
                (spellInfo->FacingCasterFlags & SPELL_FACING_FLAG_INFRONT))
            {
                sServerFacade->SetFacingTo(bot, spellTarget);
            }

            // Wait for spell cast
            YieldThread(GetReactDelay());
            return;
        }
    }

    // Update internal AI
    UpdateAIInternal(elapsed, minimal);
    YieldThread();
}

void PlayerbotAI::UpdateAIInternal([[maybe_unused]] uint32 elapsed, bool minimal)
{
    if (bot->IsBeingTeleported() || !bot->IsInWorld())
        return;

    ExternalEventHelper helper(_aiObjectContext);

    // logout if logout timer is ready or if instant logout is possible
    if (bot->GetSession()->isLogingOut())
    {
        WorldSession* botWorldSessionPtr = bot->GetSession();
        bool logout = botWorldSessionPtr->ShouldLogOut(time(nullptr));
        if (!master || !master->GetSession()->GetPlayer())
            logout = true;

        if (bot->HasFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_RESTING) || bot->HasUnitState(UNIT_STATE_IN_FLIGHT) ||
            botWorldSessionPtr->GetSecurity() >= AccountTypes::SEC_PLAYER)
        {
            logout = true;
        }

        if (master &&
            (master->HasFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_RESTING) || master->HasUnitState(UNIT_STATE_IN_FLIGHT) ||
                (master->GetSession() &&
                    master->GetSession()->GetSecurity() >= AccountTypes::SEC_PLAYER)))
        {
            logout = true;
        }

        if (logout)
        {
        }

        SetNextCheckDelay(sPlayerbotAIConfig->reactDelay);
        return;
    }

    botOutgoingPacketHandlers.Handle(helper);
    masterIncomingPacketHandlers.Handle(helper);
    masterOutgoingPacketHandlers.Handle(helper);

    DoNextAction(minimal);
}

bool PlayerbotAI::DoSpecificAction(std::string const name, Event event, bool silent, std::string const qualifier)
{
    std::ostringstream out;

    for (uint8 i = 0; i < BOT_STATE_MAX; i++)
    {
        ActionResult res = _engines[i]->ExecuteAction(name, event, qualifier);
        switch (res)
        {
        case ACTION_RESULT_UNKNOWN:
            continue;
        case ACTION_RESULT_OK:
            if (!silent)
            {
                //PlaySound(TEXT_EMOTE_NOD);
            }
            return true;
        case ACTION_RESULT_IMPOSSIBLE:
            out << name << ": impossible";
            if (!silent)
            {
                //TellError(out.str());
                //PlaySound(TEXT_EMOTE_NO);
            }
            return false;
        case ACTION_RESULT_USELESS:
            out << name << ": useless";
            if (!silent)
            {
                //TellError(out.str());
                //PlaySound(TEXT_EMOTE_NO);
            }
            return false;
        case ACTION_RESULT_FAILED:
            if (!silent)
            {
                //out << name << ": failed";
                //TellError(out.str());
            }
            return false;
        }
    }

    if (!silent)
    {
        //out << name << ": unknown action";
        //TellError(out.str());
    }

    return false;
}

bool PlayerbotAI::AllowActive(ActivityType activityType)
{
    // when botActiveAlone is 100% and smartScale disabled
    //if (sPlayerbotAIConfig->botActiveAlone >= 100 && !sPlayerbotAIConfig->botActiveAloneSmartScale)
    {
        return true;
    }

    // Is in combat. Always defend yourself.
    if (activityType != OUT_OF_PARTY_ACTIVITY && activityType != PACKET_ACTIVITY)
    {
        if (bot->IsInCombat())
        {
            return true;
        }
    }
    /*
    // only keep updating till initializing time has completed,
    // which prevents unneeded expensive GameTime calls.
    if (_isBotInitializing)
    {
        _isBotInitializing = GameTime::GetUptime().count() < sPlayerbotAIConfig->maxRandomBots * 0.11;

        // no activity allowed during bot initialization
        if (_isBotInitializing)
        {
            return false;
        }
    }

    // General exceptions
    if (activityType == PACKET_ACTIVITY)
    {
        return true;
    }

    // bg, raid, dungeon
    if (!WorldPosition(bot).isOverworld())
    {
        return true;
    }*/

    // bot map has active players.
    /*if (sPlayerbotAIConfig->BotActiveAloneForceWhenInMap)
    {
        if (HasRealPlayers(bot->GetMap()))
        {
            return true;
        }
    }*/

    // bot zone has active players.
    /*if (sPlayerbotAIConfig->BotActiveAloneForceWhenInZone)
    {
        if (ZoneHasRealPlayers(bot))
        {
            return true;
        }
    }

    // when in real guild
    if (sPlayerbotAIConfig->BotActiveAloneForceWhenInGuild)
    {
        if (IsInRealGuild())
        {
            return true;
        }
    }

    // Player is near. Always active.
    if (HasPlayerNearby(sPlayerbotAIConfig->BotActiveAloneForceWhenInRadius))
    {
        return true;
    }

    // Has player master. Always active.
    if (GetMaster())
    {
        PlayerbotAI* masterBotAI = GET_PLAYERBOT_AI(GetMaster());
        if (!masterBotAI || masterBotAI->IsRealPlayer())
        {
            return true;
        }
    }

    // if grouped up
    Group* group = bot->GetGroup();
    if (group)
    {
        for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
        {
            Player* member = gref->GetSource();
            if (!member || !member->IsInWorld() && member->GetMapId() != bot->GetMapId())
            {
                continue;
            }

            if (member == bot)
            {
                continue;
            }

            PlayerbotAI* memberBotAI = GET_PLAYERBOT_AI(member);
            {
                if (!memberBotAI || memberBotAI->HasRealPlayerMaster())
                {
                    return true;
                }
            }

            if (group->IsLeader(member->GetGUID()))
            {
                if (!memberBotAI->AllowActivity(PARTY_ACTIVITY))
                {
                    return false;
                }
            }
        }
    }

    // In bg queue. Speed up bg queue/join.
    if (bot->InBattlegroundQueue())
    {
        return true;
    }

    bool isLFG = false;
    if (group)
    {
        if (sLFGMgr->GetState(group->GetGUID()) != lfg::LFG_STATE_NONE)
        {
            isLFG = true;
        }
    }
    if (sLFGMgr->GetState(bot->GetGUID()) != lfg::LFG_STATE_NONE)
    {
        isLFG = true;
    }
    if (isLFG)
    {
        return true;
    }

    // HasFriend
    if (sPlayerbotAIConfig->BotActiveAloneForceWhenIsFriend)
    {
        for (auto& player : sRandomPlayerbotMgr->GetPlayers())
        {
            if (!player || !player->IsInWorld() || !player->GetSocial() || !bot->GetGUID())
            {
                continue;
            }

            if (player->GetSocial()->HasFriend(bot->GetGUID()))
            {
                return true;
            }
        }
    }

    // Force the bots to spread
    if (activityType == OUT_OF_PARTY_ACTIVITY || activityType == GRIND_ACTIVITY)
    {
        if (HasManyPlayersNearby(10, 40))
        {
            return true;
        }
    }

    // Bots don't need to move using PathGenerator.
    if (activityType == DETAILED_MOVE_ACTIVITY)
    {
        return false;
    }

    if (sPlayerbotAIConfig->botActiveAlone <= 0)
    {
        return false;
    }

    // #######################################################################################
    // All mandatory conditations are checked to be active or not, from here the remaining
    // situations are usable for scaling when enabled.
    // #######################################################################################

    // Below is code to have a specified % of bots active at all times.
    // The default is 10%. With 0.1% of all bots going active or inactive each minute.
    uint32 mod = sPlayerbotAIConfig->botActiveAlone > 100 ? 100 : sPlayerbotAIConfig->botActiveAlone;
    if (sPlayerbotAIConfig->botActiveAloneSmartScale &&
        bot->GetLevel() >= sPlayerbotAIConfig->botActiveAloneSmartScaleWhenMinLevel &&
        bot->GetLevel() <= sPlayerbotAIConfig->botActiveAloneSmartScaleWhenMaxLevel)
    {
        mod = AutoScaleActivity(mod);
    }

    uint32 ActivityNumber =
        GetFixedBotNumer(BotTypeNumber::ACTIVITY_TYPE_NUMBER, 100,
            sPlayerbotAIConfig->botActiveAlone * static_cast<float>(mod) / 100 * 0.01f);

    return ActivityNumber <=
        (sPlayerbotAIConfig->botActiveAlone * mod) /
        100;  // The given percentage of bots should be active and rotate 1% of those active bots each minute.
    */
}

bool PlayerbotAI::AllowActivity(ActivityType activityType, bool checkNow)
{
    if (!_allowActiveCheckTimer[activityType])
        _allowActiveCheckTimer[activityType] = time(nullptr);

    if (!checkNow && time(nullptr) < (_allowActiveCheckTimer[activityType] + 5))
        return _allowActive[activityType];

    bool allowed = AllowActive(activityType);
    _allowActive[activityType] = allowed;
    _allowActiveCheckTimer[activityType] = time(nullptr);
    return allowed;
}

void PlayerbotAI::Reset(bool full)
{
    if (bot->HasUnitState(UNIT_STATE_IN_FLIGHT))
        return;

    WorldSession* botWorldSessionPtr = bot->GetSession();
    bool logout = botWorldSessionPtr->ShouldLogOut(time(nullptr));

    // cancel logout
    if (!logout && bot->GetSession()->isLogingOut())
    {
        //WorldPackets::Character::LogoutCancel data = WorldPacket(CMSG_LOGOUT_CANCEL);
        //bot->GetSession()->HandleLogoutCancelOpcode(data);
        //TellMaster("Logout cancelled!");
    }

    _currentEngine = _engines[BOT_STATE_NON_COMBAT];
    _currentState = BOT_STATE_NON_COMBAT;
    nextAICheckDelay = 0;
    //whispers.clear();

    _aiObjectContext->GetValue<Unit*>("old target")->Set(nullptr);
    _aiObjectContext->GetValue<Unit*>("current target")->Set(nullptr);
    _aiObjectContext->GetValue<GuidVector>("prioritized targets")->Reset();
    _aiObjectContext->GetValue<ObjectGuid>("pull target")->Set(ObjectGuid::Empty);
    //_aiObjectContext->GetValue<GuidPosition>("rpg target")->Set(GuidPosition());
    //_aiObjectContext->GetValue<LootObject>("loot target")->Set(LootObject());
    //_aiObjectContext->GetValue<uint32>("lfg proposal")->Set(0);
    bot->SetTarget(ObjectGuid::Empty);

    LastSpellCast& lastSpell = _aiObjectContext->GetValue<LastSpellCast&>("last spell cast")->Get();
    lastSpell.Reset();

    if (full)
    {
        _aiObjectContext->GetValue<LastMovement&>("last movement")->Get().Set(nullptr);
        //_aiObjectContext->GetValue<LastMovement&>("last area trigger")->Get().Set(nullptr);
        //_aiObjectContext->GetValue<LastMovement&>("last taxi")->Get().Set(nullptr);
        //_aiObjectContext->GetValue<TravelTarget*>("travel target")->Get()->setTarget(sTravelMgr->nullTravelDestination, sTravelMgr->nullWorldPosition, true);
        //_aiObjectContext->GetValue<TravelTarget*>("travel target")->Get()->setStatus(TRAVEL_STATUS_EXPIRED);
        //_aiObjectContext->GetValue<TravelTarget*>("travel target")->Get()->setExpireIn(1000);
        _rpgInfo = NewRpgInfo();
    }

    _aiObjectContext->GetValue<GuidSet&>("ignore rpg target")->Get().clear();

    bot->GetMotionMaster()->Clear();

    InterruptSpell();

    if (full)
    {
        for (uint8 i = 0; i < BOT_STATE_MAX; i++)
        {
            _engines[i]->Init();
        }
    }
}

void PlayerbotAI::ReInitCurrentEngine()
{
    // InterruptSpell();
    _currentEngine->Init();
}

void PlayerbotAI::ChangeEngine(BotState type)
{
    Engine* engine = _engines[type];

    if (_currentEngine != engine)
    {
        _currentEngine = engine;
        _currentState = type;
        ReInitCurrentEngine();

        switch (type)
        {
        case BOT_STATE_COMBAT:
            //TC_LOG_DEBUG("playerbots",  "=== %s COMBAT ===", bot->GetName().c_str());
            break;
        case BOT_STATE_NON_COMBAT:
            //TC_LOG_DEBUG("playerbots",  "=== %s NON-COMBAT ===", bot->GetName().c_str());
            break;
        case BOT_STATE_DEAD:
            //TC_LOG_DEBUG("playerbots",  "=== %s DEAD ===", bot->GetName().c_str());
            break;
        default:
            break;
        }
    }
}
void PlayerbotAI::ChangeStrategy(std::string const names, BotState type)
{
    Engine* e = _engines[type];
    if (!e)
        return;

    e->ChangeStrategy(names);
}

void PlayerbotAI::ClearStrategies(BotState type)
{
    Engine* e = _engines[type];
    if (!e)
        return;

    e->removeAllStrategies();
}

std::vector<std::string> PlayerbotAI::GetStrategies(BotState type)
{
    Engine* e = _engines[type];
    if (!e)
        return std::vector<std::string>();

    return e->GetStrategies();
}

void PlayerbotAI::DoNextAction(bool min)
{
    if (!bot->IsInWorld() || bot->IsBeingTeleported() || (GetMaster() && GetMaster()->IsBeingTeleported()))
    {
        SetNextCheckDelay(sPlayerbotAIConfig->globalCoolDown);
        return;
    }

    if (bot->HasUnitState(UNIT_STATE_IN_FLIGHT))
    {
        SetNextCheckDelay(sPlayerbotAIConfig->passiveDelay);
        return;
    }

    // Change engine if just died
    bool isBotAlive = bot->IsAlive();
    if (_currentEngine != _engines[BOT_STATE_DEAD] && !isBotAlive)
    {
        bot->StopMoving();
        bot->GetMotionMaster()->Clear();
        bot->GetMotionMaster()->MoveIdle();

        // Death Count to prevent skeleton piles
        Player* master = GetMaster();  // warning here - whipowill
        if (!HasActivePlayerMaster() && !bot->InBattleground())
        {
            uint32 dCount = _aiObjectContext->GetValue<uint32>("death count")->Get();
            _aiObjectContext->GetValue<uint32>("death count")->Set(++dCount);
        }

        _aiObjectContext->GetValue<Unit*>("current target")->Set(nullptr);
        //_aiObjectContext->GetValue<Unit*>("enemy player target")->Set(nullptr);
        //_aiObjectContext->GetValue<ObjectGuid>("pull target")->Set(ObjectGuid::Empty);
        //_aiObjectContext->GetValue<LootObject>("loot target")->Set(LootObject());

        ChangeEngine(BOT_STATE_DEAD);
        return;
    }

    // Change engine if just ressed
    if (_currentEngine == _engines[BOT_STATE_DEAD] && isBotAlive)
    {
        ChangeEngine(BOT_STATE_NON_COMBAT);
        return;
    }

    // Clear targets if in combat but sticking with old data
    if (_currentEngine == _engines[BOT_STATE_NON_COMBAT] && bot->IsInCombat())
    {
        Unit* currentTarget = _aiObjectContext->GetValue<Unit*>("current target")->Get();
        if (currentTarget != nullptr)
        {
            _aiObjectContext->GetValue<Unit*>("current target")->Set(nullptr);
        }
    }

    bool minimal = !AllowActivity();
    _currentEngine->DoNextAction(nullptr, 0, (minimal || min));

    if (minimal)
    {
        if (!bot->isAFK() && !bot->InBattleground() && !HasRealPlayerMaster())
            bot->ToggleAFK();

        SetNextCheckDelay(sPlayerbotAIConfig->passiveDelay);
        return;
    }
    else if (bot->isAFK())
        bot->ToggleAFK();

    Group* group = bot->GetGroup();
    PlayerbotAI* masterBotAI = nullptr;
    if (master)
        masterBotAI = GET_PLAYERBOT_AI(master);

    if (bot->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_WALKING))
        bot->m_movementInfo.RemoveMovementFlag(MOVEMENTFLAG_WALKING);
    else if ((nextAICheckDelay < 1000) && bot->IsSitState())
        bot->SetStandState(UNIT_STAND_STATE_STAND);

    bool hasMountAura = bot->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED) ||
        bot->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED);
    if (hasMountAura && !bot->IsMounted())
    {
        bot->RemoveAurasByType(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED);
        bot->RemoveAurasByType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED);
    }
}

void PlayerbotAI::HandleTeleportAck()
{
    if (IsRealPlayer())
        return;

    bot->GetMotionMaster()->Clear(true);
    bot->StopMoving();
    if (bot->IsBeingTeleportedNear())
    {
        // Temporary fix for instance can not enter
        if (!bot->IsInWorld())
        {
            bot->GetMap()->AddPlayerToMap(bot);
        }
        while (bot->IsInWorld() && bot->IsBeingTeleportedNear())
        {
            Player* plMover = bot->m_mover->ToPlayer();
            if (!plMover)
                return;

            ObjectGuid guid = plMover->GetGUID();
            WorldPacket p = WorldPacket(CMSG_MOVE_TELEPORT_ACK, 20);
            p << (uint32)0;  // supposed to be flags? not used currently
            p << (uint32)0;  // time - not currently used
            
            p.WriteBit(guid[0]);
            p.WriteBit(guid[7]);
            p.WriteBit(guid[3]);
            p.WriteBit(guid[5]);
            p.WriteBit(guid[4]);
            p.WriteBit(guid[6]);
            p.WriteBit(guid[1]);
            p.WriteBit(guid[2]);

            p.WriteByteSeq(guid[4]);
            p.WriteByteSeq(guid[1]);
            p.WriteByteSeq(guid[6]);
            p.WriteByteSeq(guid[7]);
            p.WriteByteSeq(guid[0]);
            p.WriteByteSeq(guid[2]);
            p.WriteByteSeq(guid[5]);
            p.WriteByteSeq(guid[3]);
            
            bot->GetSession()->HandleMoveTeleportAck(p);
        };
    }
    if (bot->IsBeingTeleportedFar())
    {
        /*while (bot->IsBeingTeleportedFar())
        {
            bot->GetSession()->HandleMoveWorldportAck();
        }
        // SetNextCheckDelay(urand(2000, 5000));
        if (sPlayerbotAIConfig->applyInstanceStrategies)
            ApplyInstanceStrategies(bot->GetMapId(), true);
        Reset(true);
    }*/
    }
    SetNextCheckDelay(sPlayerbotAIConfig->globalCoolDown);
}

void PlayerbotAI::HandleBotOutgoingPacket(WorldPacket const& packet)
{
    if (packet.empty())
        return;
    if (!bot || !bot->IsInWorld() || bot->IsDuringRemoveFromWorld())
    {
        return;
    }

    //TC_LOG_INFO("playerbots", "Player: %s Received packet %s", bot->GetName().c_str(), GetOpcodeNameForLogging((OpcodeServer)packet->GetOpcode()).c_str());

    switch (packet.GetOpcode())
    {
    case SMSG_SPELL_FAILURE:
    {
        return;
    }
    case SMSG_SPELL_DELAYED:
    {
        return;
    }
    case SMSG_EMOTE:  // do not react to NPC emotes
    {
        return;
    }
    case SMSG_MESSAGECHAT:  // do not react to self or if not ready to reply
    {
        return;
    }
    case SMSG_MOVE_KNOCK_BACK:  // handle knockbacks
    {
        return;
    }
    case SMSG_TIME_SYNC_REQ:
    {
        WorldPacket p = packet;
        uint32 counter;
        p >> counter;
        uint32 clientTicks = time(NULL);
        WorldPacket packet(CMSG_TIME_SYNC_RESP);
        packet.rpos(0);
        packet << counter << clientTicks;

        bot->GetSession()->HandleTimeSyncResp(packet);
        break;
    }
    default:
        botOutgoingPacketHandlers.AddPacket(packet);
        return;
    }
}

void PlayerbotAI::HandleMasterIncomingPacket(WorldPacket const& packet)
{
    masterIncomingPacketHandlers.AddPacket(packet);
}

void PlayerbotAI::HandleMasterOutgoingPacket(WorldPacket const& packet)
{
    masterOutgoingPacketHandlers.AddPacket(packet);
}

bool PlayerbotAI::HasRealPlayerMaster()
{
    if (master)
    {
        PlayerbotAI* masterBotAI = GET_PLAYERBOT_AI(master);
        return !masterBotAI || masterBotAI->IsRealPlayer();
    }

    return false;
}
bool PlayerbotAI::HasActivePlayerMaster()
{
    return master && !GET_PLAYERBOT_AI(master);
}

bool PlayerbotAI::IsAlt()
{
    return HasRealPlayerMaster() && !sRandomPlayerbotMgr->IsRandomBot(bot);
}

bool PlayerbotAI::IsInVehicle(bool canControl, bool canCast, bool canAttack, bool canTurn, bool fixed)
{
    Vehicle* vehicle = bot->GetVehicle();
    if (!vehicle)
        return false;

    // get vehicle
    Unit* vehicleBase = vehicle->GetBase();
    if (!vehicleBase || !vehicleBase->IsAlive())
        return false;

    if (!vehicle->GetVehicleInfo())
        return false;

    // get seat
    VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(bot);
    if (!seat)
        return false;

    if (!(canControl || canCast || canAttack || canTurn || fixed))
        return true;

    if (canControl)
        return seat->CanControl() && !(vehicle->GetVehicleInfo()->m_flags & VEHICLE_FLAG_FIXED_POSITION);

    if (canCast)
        return (seat->m_flags & VEHICLE_SEAT_FLAG_CAN_CAST) != 0;

    if (canAttack)
        return (seat->m_flags & VEHICLE_SEAT_FLAG_CAN_ATTACK) != 0;

    if (canTurn)
        return (seat->m_flags & VEHICLE_SEAT_FLAG_ALLOW_TURNING) != 0;

    if (fixed)
        return (vehicle->GetVehicleInfo()->m_flags & VEHICLE_FLAG_FIXED_POSITION) != 0;

    return false;
}
bool PlayerbotAI::IsOpposing(Player* player)
{
    return IsOpposing(player->GetRace(), bot->GetRace());
}

bool PlayerbotAI::IsOpposing(uint8 race1, uint8 race2)
{
    return (IsAlliance(race1) && !IsAlliance(race2)) || (!IsAlliance(race1) && IsAlliance(race2));
}
bool PlayerbotAI::HasStrategy(std::string const name, BotState type)
{
    if (_engines[type])
        return _engines[type]->HasStrategy(name);
    return false;
}

bool PlayerbotAI::ContainsStrategy(StrategyType type)
{
    for (uint8 i = 0; i < BOT_STATE_MAX; i++)
    {
        if (_engines[i]->HasStrategyType(type))
            return true;
    }

    return false;
}

bool PlayerbotAI::CanMove()
{
    // do not allow if not vehicle driver
    //if (IsInVehicle() && !IsInVehicle(true))
        //return false;

    if (bot->isFrozen() || bot->IsPolymorphed() || (bot->isDead() && !bot->HasFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_GHOST)) ||
        bot->IsBeingTeleported() /* || bot->HasRootAura() || bot->HasSpiritOfRedemptionAura() || bot->HasConfuseAura()*/ ||
        bot->IsCharmed() /* || bot->HasStunAura()*/ || bot->IsInFlight() || bot->HasUnitState(UNIT_STATE_LOST_CONTROL))
        return false;

    return bot->GetMotionMaster()->GetCurrentMovementGeneratorType() != FLIGHT_MOTION_TYPE;
}

Unit* PlayerbotAI::GetUnit(ObjectGuid guid)
{
    if (!guid)
        return nullptr;

    return ObjectAccessor::GetUnit(*bot, guid);
}

Player* PlayerbotAI::GetPlayer(ObjectGuid guid)
{
    Unit* unit = GetUnit(guid);
    return unit ? unit->ToPlayer() : nullptr;
}

Player* PlayerbotAI::GetGroupMaster()
{
    if (!bot->InBattleground())
        if (Group* group = bot->GetGroup())
            if (Player* player = ObjectAccessor::FindPlayer(group->GetLeaderGUID()))
                return player;

    return master;
}

uint32 GetCreatureIdForCreatureTemplateId(uint32 creatureTemplateId)
{
    QueryResult results = WorldDatabase.PQuery("SELECT guid FROM `creature` WHERE id = %u LIMIT 1;", creatureTemplateId);
    if (results)
    {
        Field* fields = results->Fetch();
        return fields[0].GetUInt32();
    }
    return 0;
}

Unit* PlayerbotAI::GetUnit(CreatureData const* creatureData)
{
    if (!creatureData)
        return nullptr;

    Map* map = sMapMgr->FindMap(creatureData->mapId, 0);
    if (!map)
        return nullptr;

    uint32 spawnId = creatureData->spawnId;
    if (!spawnId)  // workaround for CreatureData with missing spawnId (this just uses first matching creatureId in DB,
        // but thats ok this method is only used for battlemasters and theres only 1 of each type)
        spawnId = GetCreatureIdForCreatureTemplateId(creatureData->id);
    auto creatureBounds = map->GetCreatureBySpawnIdStore().equal_range(spawnId);
    if (creatureBounds.first == creatureBounds.second)
        return nullptr;

    return creatureBounds.first->second;
}

Creature* PlayerbotAI::GetCreature(ObjectGuid guid)
{
    if (!guid)
        return nullptr;

    return ObjectAccessor::GetCreature(*bot, guid);
}

GameObject* PlayerbotAI::GetGameObject(ObjectGuid guid)
{
    if (!guid)
        return nullptr;

    return ObjectAccessor::GetGameObject(*bot, guid);
}
WorldObject* PlayerbotAI::GetWorldObject(ObjectGuid guid)
{
    if (!guid)
        return nullptr;

    return ObjectAccessor::GetWorldObject(*bot, guid);
}

bool PlayerbotAI::SayToParty(const std::string& msg)
{
    if (!bot->GetGroup())
        return false;

    /*WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_PARTY, msg.c_str(), LANG_UNIVERSAL, CHAT_TAG_NONE, bot->GetGUID(),
        bot->GetName());

    for (auto reciever : GetPlayersInGroup())
    {
        sServerFacade->SendPacket(reciever, &data);
    }*/

    return true;
}

bool PlayerbotAI::SayToRaid(const std::string& msg)
{
    if (!bot->GetGroup() || bot->GetGroup()->isRaidGroup())
        return false;

    /*WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_RAID, msg.c_str(), LANG_UNIVERSAL, CHAT_TAG_NONE, bot->GetGUID(),
        bot->GetName());

    for (auto reciever : GetPlayersInGroup())
    {
        sServerFacade->SendPacket(reciever, &data);
    }*/

    return true;
}

bool PlayerbotAI::Yell(const std::string& msg)
{
    if (bot->GetTeamId() == TeamId::TEAM_ALLIANCE)
    {
        bot->Yell(msg, LANG_COMMON);
    }
    else
    {
        bot->Yell(msg, LANG_ORCISH);
    }

    return true;
}

bool PlayerbotAI::Say(const std::string& msg)
{
    if (bot->GetTeamId() == TeamId::TEAM_ALLIANCE)
    {
        bot->Say(msg, LANG_COMMON);
    }
    else
    {
        bot->Say(msg, LANG_ORCISH);
    }

    return true;
}

bool PlayerbotAI::Whisper(const std::string& msg, const std::string& receiverName)
{
    const auto receiver = ObjectAccessor::FindPlayerByName(receiverName);
    if (!receiver)
    {
        return false;
    }

    if (bot->GetTeamId() == TeamId::TEAM_ALLIANCE)
    {
        bot->Whisper(msg, LANG_COMMON, receiver);
    }
    else
    {
        bot->Whisper(msg, LANG_ORCISH, receiver);
    }

    return true;
}

bool PlayerbotAI::TellMasterNoFacing(std::ostringstream& stream)
{
    return TellMasterNoFacing(stream.str());
}

bool PlayerbotAI::TellMasterNoFacing(std::string const text)
{
    Player* master = GetMaster();
    PlayerbotAI* masterBotAI = nullptr;
    if (master)
        masterBotAI = GET_PLAYERBOT_AI(master);

    /*if ((!master || (masterBotAI && !masterBotAI->IsRealPlayer())) &&
        (sPlayerbotAIConfig->randomBotSayWithoutMaster || HasStrategy("debug", BOT_STATE_NON_COMBAT)))
    {
        bot->Say(text, (bot->GetTeamId() == TEAM_ALLIANCE ? LANG_COMMON : LANG_ORCISH));
        return true;
    }

    if (!IsTellAllowed(securityLevel))
        return false;

    time_t lastSaid = whispers[text];

    if (!lastSaid || (time(nullptr) - lastSaid) >= sPlayerbotAIConfig->repeatDelay / 1000)
    {
        whispers[text] = time(nullptr);

        ChatMsg type = CHAT_MSG_WHISPER;
        if (currentChat.second - time(nullptr) >= 1)
            type = currentChat.first;

        WorldPacket data;
        ChatHandler::BuildChatPacket(data, type == CHAT_MSG_ADDON ? CHAT_MSG_PARTY : type,
            type == CHAT_MSG_ADDON ? LANG_ADDON : LANG_UNIVERSAL, bot, nullptr, text.c_str());
        master->SendDirectMessage(&data);
    }*/

    return true;
}
bool PlayerbotAI::TellError(std::string const text)
{
    Player* master = GetMaster();
    if (!master || GET_PLAYERBOT_AI(master))
        return false;

    if (PlayerbotMgr* mgr = GET_PLAYERBOT_MGR(master))
        mgr->TellError(bot->GetName(), text);

    return false;
}

float PlayerbotAI::GetRange(std::string const type)
{
    float val = 0;
    if (_aiObjectContext)
        val = _aiObjectContext->GetValue<float>("range", type)->Get();

    if (abs(val) >= 0.1f)
        return val;

    if (type == "spell")
        return sPlayerbotAIConfig->spellDistance;

    if (type == "shoot")
        return sPlayerbotAIConfig->shootDistance;

    if (type == "flee")
        return sPlayerbotAIConfig->fleeDistance;

    if (type == "heal")
        return sPlayerbotAIConfig->healDistance;

    if (type == "melee")
        return sPlayerbotAIConfig->meleeDistance;

    return 0;
}

bool IsRealAura(Player* bot, AuraEffect const* aurEff, Unit const* unit)
{
    if (!aurEff)
        return false;

    if (!unit->IsHostileTo(bot))
        return true;

    SpellInfo const* spellInfo = aurEff->GetSpellInfo();

    uint32 stacks = aurEff->GetBase()->GetStackAmount();
    if (stacks >= spellInfo->StackAmount)
        return true;

    if (aurEff->GetCaster() == bot || spellInfo->IsPositive() ||
        spellInfo->Effects[aurEff->GetEffIndex()].IsAreaAuraEffect())
        return true;

    return false;
}

bool PlayerbotAI::HasAura(std::string const name, Unit* unit, bool maxStack, bool checkIsOwner, int maxAuraAmount,
    bool checkDuration)
{
    if (!unit)
        return false;

    std::wstring wnamepart;
    if (!Utf8toWStr(name, wnamepart))
        return false;

    wstrToLower(wnamepart);

    int auraAmount = 0;

    // Iterate through all aura types
    for (uint32 auraType = SPELL_AURA_BIND_SIGHT; auraType < TOTAL_AURAS; auraType++)
    {
        Unit::AuraEffectList const& auras = unit->GetAuraEffectsByType((AuraType)auraType);
        if (auras.empty())
            continue;

        // Iterate through each aura effect
        for (AuraEffect const* aurEff : auras)
        {
            if (!aurEff)
                continue;

            SpellInfo const* spellInfo = aurEff->GetSpellInfo();
            if (!spellInfo)
                continue;

            // Check if the aura name matches
            std::string_view const auraName = spellInfo->SpellName[0];
            if (auraName.empty() || auraName.length() != wnamepart.length() || !Utf8FitTo(std::string(auraName), wnamepart))
                continue;

            // Check if this is a valid aura for the bot
            if (IsRealAura(bot, aurEff, unit))
            {
                // Check caster if necessary
                if (checkIsOwner && aurEff->GetCasterGUID() != bot->GetGUID())
                    continue;

                // Check aura duration if necessary
                if (checkDuration && aurEff->GetBase()->GetDuration() == -1)
                    continue;

                // Count stacks and charges
                uint32 maxStackAmount = spellInfo->StackAmount;
                uint32 maxProcCharges = spellInfo->ProcCharges;

                // Count the aura based on max stack and proc charges
                if (maxStack)
                {
                    if (maxStackAmount && aurEff->GetBase()->GetStackAmount() >= maxStackAmount)
                        auraAmount++;

                    if (maxProcCharges && aurEff->GetBase()->GetCharges() >= maxProcCharges)
                        auraAmount++;
                }
                else
                {
                    auraAmount++;
                }

                // Early exit if maxAuraAmount is reached
                if (maxAuraAmount < 0 && auraAmount > 0)
                    return true;
            }
        }
    }

    // Return based on the maximum aura amount conditions
    if (maxAuraAmount >= 0)
    {
        return auraAmount == maxAuraAmount || (auraAmount > 0 && auraAmount <= maxAuraAmount);
    }

    return false;
}

bool PlayerbotAI::HasAura(uint32 spellId, Unit const* unit)
{
    if (!spellId || !unit)
        return false;

    return unit->HasAura(spellId);
    // for (uint8 effect = EFFECT_0; effect <= EFFECT_2; effect++)
    // {
    // 	AuraEffect const* aurEff = unit->GetAuraEffect(spellId, effect);
    // 	if (IsRealAura(bot, aurEff, unit))
    // 		return true;
    // }

    // return false;
}

void PlayerbotAI::RemoveShapeshift()
{
    RemoveAura("bear form");
    RemoveAura("dire bear form");
    RemoveAura("moonkin form");
    RemoveAura("travel form");
    RemoveAura("cat form");
    RemoveAura("flight form");
    RemoveAura("swift flight form");
    RemoveAura("aquatic form");
    RemoveAura("ghost wolf");
    // RemoveAura("tree of life");
}

void PlayerbotAI::RemoveAura(std::string const name)
{
    uint32 spellid = _aiObjectContext->GetValue<uint32>("spell id", name)->Get();
    if (spellid && HasAura(spellid, bot))
        bot->RemoveAurasDueToSpell(spellid);
}

Aura* PlayerbotAI::GetAura(std::string const name, Unit* unit, bool checkIsOwner, bool checkDuration, int checkStack)
{
    if (!unit)
        return nullptr;

    std::wstring wnamepart;
    if (!Utf8toWStr(name, wnamepart))
        return nullptr;

    wstrToLower(wnamepart);

    for (uint32 auraType = SPELL_AURA_BIND_SIGHT; auraType < TOTAL_AURAS; ++auraType)
    {
        Unit::AuraEffectList const& auras = unit->GetAuraEffectsByType((AuraType)auraType);
        if (auras.empty())
            continue;

        for (AuraEffect const* aurEff : auras)
        {
            SpellInfo const* spellInfo = aurEff->GetSpellInfo();
            std::string const& auraName = spellInfo->SpellName[0];

            // Directly skip if name mismatch (both length and content)
            if (auraName.empty() || auraName.length() != wnamepart.length() || !Utf8FitTo(auraName, wnamepart))
                continue;

            if (!IsRealAura(bot, aurEff, unit))
                continue;

            // Check owner if necessary
            if (checkIsOwner && aurEff->GetCasterGUID() != bot->GetGUID())
                continue;

            // Check duration if necessary
            if (checkDuration && aurEff->GetBase()->GetDuration() == -1)
                continue;

            // Check stack if necessary
            if (checkStack != -1 && aurEff->GetBase()->GetStackAmount() < checkStack)
                continue;

            return aurEff->GetBase();
        }
    }

    return nullptr;
}

bool PlayerbotAI::HasAnyAuraOf(Unit* player, ...)
{
    if (!player)
        return false;

    va_list vl;
    va_start(vl, player);

    const char* cur;
    while ((cur = va_arg(vl, const char*)) != nullptr)
    {
        if (HasAura(cur, player))
        {
            va_end(vl);
            return true;
        }
    }

    va_end(vl);
    return false;
}

void PlayerbotAI::SpellInterrupted(uint32 spellid)
{
    for (uint8 type = CURRENT_MELEE_SPELL; type <= CURRENT_CHANNELED_SPELL; type++)
    {
        Spell* spell = bot->GetCurrentSpell((CurrentSpellTypes)type);
        if (!spell)
            continue;
        if (spell->GetSpellInfo()->Id == spellid)
            bot->InterruptSpell((CurrentSpellTypes)type);
    }
}

void PlayerbotAI::InterruptSpell()
{
    for (uint8 type = CURRENT_MELEE_SPELL; type <= CURRENT_CHANNELED_SPELL; type++)
    {
        Spell* spell = bot->GetCurrentSpell((CurrentSpellTypes)type);
        if (!spell)
            continue;

        bot->InterruptSpell((CurrentSpellTypes)type);

        WorldPacket data(SMSG_SPELL_FAILURE, 8 + 1 + 4 + 1);
        data << bot->GetPackGUID();
        data << uint8(1);
        data << uint32(spell->m_spellInfo->Id);
        data << uint8(0);
        bot->SendMessageToSet(&data, true);

        data.Initialize(SMSG_SPELL_FAILED_OTHER, 8 + 1 + 4 + 1);
        data << bot->GetPackGUID();
        data << uint8(1);
        data << uint32(spell->m_spellInfo->Id);
        data << uint8(0);
        bot->SendMessageToSet(&data, true);

        SpellInterrupted(spell->m_spellInfo->Id);
    }
}

bool PlayerbotAI::CanCastSpell(std::string const name, Unit* target, Item* itemTarget)
{
    return CanCastSpell(_aiObjectContext->GetValue<uint32>("spell id", name)->Get(), target, true, itemTarget);
}

bool PlayerbotAI::CanCastSpell(uint32 spellid, Unit* target, bool checkHasSpell, Item* itemTarget, Item* castItem)
{
    if (!spellid)
    {
        //if (!sPlayerbotAIConfig->logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
        {
            //TC_LOG_DEBUG("playerbots", "Can cast spell failed. No spellid. - spellid: %u, bot name: %s", spellid, bot->GetName().c_str());
        }
        return false;
    }

    if (bot->HasUnitState(UNIT_STATE_LOST_CONTROL))
    {
        //if (!sPlayerbotAIConfig->logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
        {
            //TC_LOG_DEBUG("playerbots", "Can cast spell failed. Unit state lost control. - spellid: %u, bot name: %s", spellid, bot->GetName().c_str());
        }
        return false;
    }

    if (!target)
        target = bot;

    // if (!sPlayerbotAIConfig->logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
    //     LOG_DEBUG("playerbots", "Can cast spell? - target name: {}, spellid: {}, bot name: {}",
    //             target->GetName(), spellid, bot->GetName());

    if (Pet* pet = bot->GetPet())
        if (pet->HasSpell(spellid))
            return true;

    if (checkHasSpell && !bot->HasSpell(spellid))
    {
        //if (!sPlayerbotAIConfig->logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
        {
            //TC_LOG_DEBUG("playerbots", "Can cast spell failed. Bot not has spell. - target name: %s, spellid: %u, bot name: %s", target->GetName().c_str(), spellid, bot->GetName().c_str());
        }
        return false;
    }

    if (bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL) != nullptr)
    {
        //if (!sPlayerbotAIConfig->logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
        {
            //TC_LOG_DEBUG(
            //    "playerbots",
            //    "CanCastSpell() target name: %d, spellid: %u, bot name: %s, failed because has current channeled spell",
            //    target->GetName().c_str(), spellid, bot->GetName().c_str());
        }
        return false;
    }

    if (bot->HasSpellCooldown(spellid))
    {
        //if (!sPlayerbotAIConfig->logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
        {
           // TC_LOG_DEBUG("playerbots",
            //    "Can cast spell failed. Spell not has cooldown. - target name: %s, spellid: %u, bot name: %s",
            //    target->GetName().c_str(), spellid, bot->GetName().c_str());
        }
        return false;
    }

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid);
    if (!spellInfo)
    {
        //if (!sPlayerbotAIConfig->logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
        {
            //TC_LOG_DEBUG("playerbots", "Can cast spell failed. No spellInfo. - target name: %s, spellid: %u, bot name: %s",
             //   target->GetName().c_str(), spellid, bot->GetName().c_str());
        }
        return false;
    }

    uint32 CastingTime = !spellInfo->IsChanneled() ? spellInfo->CalcCastTime(bot->GetLevel()) : spellInfo->GetDuration();
    // bool interruptOnMove = spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_MOVEMENT;
    if ((CastingTime || spellInfo->IsAutoRepeatRangedSpell()) && bot->isMoving())
    {
        //if (!sPlayerbotAIConfig->logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
        {
            //TC_LOG_DEBUG("playerbots", "Casting time and bot is moving - target name: %s, spellid: %u, bot name: %s",
             //   target->GetName().c_str(), spellid, bot->GetName().c_str());
        }
        return false;
    }

    if (!itemTarget)
    {
        bool positiveSpell = spellInfo->IsPositive();
        // if (positiveSpell && bot->IsHostileTo(target))
        //     return false;

        // if (!positiveSpell && bot->IsFriendlyTo(target))
        //     return false;

        // bool damage = false;
        // for (uint8 i = EFFECT_0; i <= EFFECT_2; i++)
        // {
        //     if (spellInfo->Effects[i].Effect == SPELL_EFFECT_SCHOOL_DAMAGE)
        //     {
        //         damage = true;
        //         break;
        //     }
        // }

        if (target->IsImmunedToSpell(spellInfo, spellInfo->GetAllEffectsMechanicMask()))
        {
            //if (!sPlayerbotAIConfig->logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
            {
               // TC_LOG_DEBUG("playerbots", "target is immuned to spell - target name: %s, spellid: %u, bot name: %s",
                //    target->GetName(), spellid, bot->GetName());
            }
            return false;
        }

        // if (!damage)
        // {
        //     for (uint8 i = EFFECT_0; i <= EFFECT_2; i++)
        //     {
        //         if (target->IsImmunedToSpellEffect(spellInfo, i)) {
        //             if (!sPlayerbotAIConfig->logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster())) {
        //                 LOG_DEBUG("playerbots", "target is immuned to spell effect - target name: {}, spellid: {},
        //                 bot name: {}",
        //                     target->GetName(), spellid, bot->GetName());
        //             }
        //             return false;
        //         }
        //     }
        // }

        if (bot != target && sServerFacade->GetDistance2d(bot, target) > sPlayerbotAIConfig->sightDistance)
        {
            //if (!sPlayerbotAIConfig->logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
            {
               // TC_LOG_DEBUG("playerbots", "target is out of sight distance - target name: %s, spellid: %u, bot name: %s",
               //     target->GetName(), spellid, bot->GetName());
            }
            return false;
        }
    }

    Unit* oldSel = bot->GetSelectedUnit();
    // TRIGGERED_IGNORE_POWER_AND_REAGENT_COST flag for not calling CheckPower in check
    // which avoids buff charge to be ineffectively reduced (e.g. dk freezing fog for howling blast)
    /// @TODO: Fix all calls to ApplySpellMod

    Spell* spell = new Spell(bot, spellInfo, TRIGGERED_IGNORE_POWER_AND_REAGENT_COST);
    spell->m_targets.SetUnitTarget(target);
    spell->m_CastItem = castItem;
    if (itemTarget == nullptr)
    {
        //itemTarget = aiObjectContext->GetValue<Item*>("item for spell", spellid)->Get();
    }
    spell->m_targets.SetItemTarget(itemTarget);
    SpellCastResult result = spell->CheckCast(true);
    delete spell;

    if (oldSel)
        bot->SetSelection(oldSel->GetGUID());

    switch (result)
    {
    case SPELL_FAILED_NOT_INFRONT:
    case SPELL_FAILED_NOT_STANDING:
    case SPELL_FAILED_UNIT_NOT_INFRONT:
    case SPELL_FAILED_MOVING:
    case SPELL_FAILED_TRY_AGAIN:
    case SPELL_CAST_OK:
    case SPELL_FAILED_NOT_SHAPESHIFT:
    case SPELL_FAILED_OUT_OF_RANGE:
        return true;
    default:
        //if (!sPlayerbotAIConfig->logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster()))
        {
            // if (result != SPELL_FAILED_NOT_READY && result != SPELL_CAST_OK) {
           // TC_LOG_DEBUG("playerbots",
            //    "CanCastSpell Check Failed. - target name: %s, spellid: %u, bot name: %s, result: %u",
            //    target->GetName().c_str(), spellid, bot->GetName().c_str(), (uint32)result);
        }
        return false;
    }
}

bool PlayerbotAI::CanCastSpell(uint32 spellid, GameObject* goTarget, uint8 effectMask, bool checkHasSpell)
{
    if (!spellid)
        return false;

    if (bot->HasUnitState(UNIT_STATE_LOST_CONTROL))
        return false;

    Pet* pet = bot->GetPet();
    if (pet && pet->HasSpell(spellid))
        return true;

    if (checkHasSpell && !bot->HasSpell(spellid))
        return false;

    if (bot->HasSpellCooldown(spellid))
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid);
    if (!spellInfo)
        return false;

    int32 CastingTime = !spellInfo->IsChanneled() ? spellInfo->CalcCastTime(bot->GetLevel()) : spellInfo->GetDuration();
    if (CastingTime > 0 && bot->isMoving())
        return false;

    bool damage = false;
    for (int32 i = EFFECT_0; i <= EFFECT_2; i++)
    {
        if (spellInfo->Effects[i].Effect == SPELL_EFFECT_SCHOOL_DAMAGE)
        {
            damage = true;
            break;
        }
    }

    if (sServerFacade->GetDistance2d(bot, goTarget) > sPlayerbotAIConfig->sightDistance)
        return false;

    // ObjectGuid oldSel = bot->GetTarget();
    // bot->SetTarget(goTarget->GetGUID());
    Spell* spell = new Spell(bot, spellInfo, TRIGGERED_NONE);

    spell->m_targets.SetGOTarget(goTarget);
    //spell->m_CastItem = aiObjectContext->GetValue<Item*>("item for spell", spellid)->Get();
    //spell->m_targets.SetItemTarget(spell->m_CastItem);

    SpellCastResult result = spell->CheckCast(true);
    delete spell;
    // if (oldSel)
    //     bot->SetTarget(oldSel);

    switch (result)
    {
    case SPELL_FAILED_NOT_INFRONT:
    case SPELL_FAILED_NOT_STANDING:
    case SPELL_FAILED_UNIT_NOT_INFRONT:
    case SPELL_FAILED_MOVING:
    case SPELL_FAILED_TRY_AGAIN:
    case SPELL_CAST_OK:
        return true;
    default:
        break;
    }

    return false;
}

bool PlayerbotAI::CanCastSpell(uint32 spellid, float x, float y, float z, uint8 effectMask, bool checkHasSpell,
    Item* itemTarget)
{
    if (!spellid)
        return false;

    Pet* pet = bot->GetPet();
    if (pet && pet->HasSpell(spellid))
        return true;

    if (checkHasSpell && !bot->HasSpell(spellid))
        return false;

    if (bot->HasSpellCooldown(spellid))
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid);
    if (!spellInfo)
        return false;

    if (!itemTarget)
    {
        if (bot->GetDistance(x, y, z) > sPlayerbotAIConfig->sightDistance)
            return false;
    }

    Spell* spell = new Spell(bot, spellInfo, TRIGGERED_NONE);

    spell->m_targets.SetDst(x, y, z, 0.f);
    //spell->m_CastItem = itemTarget ? itemTarget : aiObjectContext->GetValue<Item*>("item for spell", spellid)->Get();
    //spell->m_targets.SetItemTarget(spell->m_CastItem);

    SpellCastResult result = spell->CheckCast(true);
    delete spell;

    switch (result)
    {
    case SPELL_FAILED_NOT_INFRONT:
    case SPELL_FAILED_NOT_STANDING:
    case SPELL_FAILED_UNIT_NOT_INFRONT:
    case SPELL_FAILED_MOVING:
    case SPELL_FAILED_TRY_AGAIN:
    case SPELL_CAST_OK:
        return true;
    default:
        return false;
    }
}

bool PlayerbotAI::CastSpell(std::string const name, Unit* target, Item* itemTarget)
{
    if (name == "frostfire bolt")
        TC_LOG_DEBUG("playerbots", "frostfire bolt");
    bool result = CastSpell(_aiObjectContext->GetValue<uint32>("spell id", name)->Get(), target, itemTarget);
    if (result)
    {
        _aiObjectContext->GetValue<time_t>("last spell cast time", name)->Set(time(nullptr));
    }

    return result;
}

bool PlayerbotAI::CastSpell(uint32 spellId, Unit* target, Item* itemTarget)
{
    if (!spellId)
    {
        return false;
    }

    if (!target)
        target = bot;

    Pet* pet = bot->GetPet();
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (pet && pet->HasSpell(spellId))
    {
        bool autocast = false;
        for (unsigned int& m_autospell : pet->m_autospells)
        {
            if (m_autospell == spellId)
            {
                autocast = true;
                break;
            }
        }

        pet->ToggleAutocast(spellInfo, !autocast);
        //std::ostringstream out;
        //out << (autocast ? "|cffff0000|Disabling" : "|cFF00ff00|Enabling") << " pet auto-cast for ";
        //out << chatHelper.FormatSpell(spellInfo);
        //TellMaster(out);
        return true;
    }

    // aiObjectContext->GetValue<LastMovement&>("last movement")->Get().Set(nullptr);
    // aiObjectContext->GetValue<time_t>("stay time")->Set(0);

    if (bot->IsFlying() || bot->HasUnitState(UNIT_STATE_IN_FLIGHT))
    {
        // if (!sPlayerbotAIConfig->logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster())) {
        //     LOG_DEBUG("playerbots", "Spell cast is flying - target name: {}, spellid: {}, bot name: {}}",
        //         target->GetName(), spellId, bot->GetName());
        // }
        return false;
    }

    // bot->ClearUnitState(UNIT_STATE_CHASE);
    // bot->ClearUnitState(UNIT_STATE_FOLLOW);

    bool failWithDelay = false;
    if (!bot->IsStandState())
    {
        bot->SetStandState(UNIT_STAND_STATE_STAND);
        failWithDelay = true;
    }

    ObjectGuid oldSel = bot->GetSelectedUnit() ? bot->GetSelectedUnit()->GetGUID() : ObjectGuid();
    bot->SetSelection(target->GetGUID());
    WorldObject* faceTo = target;
    if (!bot->HasInArc(CAST_ANGLE_IN_FRONT, faceTo) && (spellInfo->FacingCasterFlags & SPELL_FACING_FLAG_INFRONT))
    {
        sServerFacade->SetFacingTo(bot, faceTo);
        // failWithDelay = true;
    }

    if (failWithDelay)
    {
        SetNextCheckDelay(sPlayerbotAIConfig->reactDelay);
        return false;
    }

    Spell* spell = new Spell(bot, spellInfo, TRIGGERED_NONE);

    SpellCastTargets targets;
    if (spellInfo->Targets & TARGET_FLAG_ITEM)
    {
        //spell->m_CastItem = itemTarget ? itemTarget : aiObjectContext->GetValue<Item*>("item for spell", spellId)->Get();
        targets.SetItemTarget(spell->m_CastItem);

        if (bot->GetTradeData())
        {
            //bot->GetTradeData()->SetSpell(spellId);
            delete spell;
            return true;
        }
    }
    else if (spellInfo->Targets & TARGET_FLAG_DEST_LOCATION)
    {
        // WorldLocation aoe = aiObjectContext->GetValue<WorldLocation>("aoe position")->Get();
        // targets.SetDst(aoe);
        targets.SetDst(*target);
    }
    else if (spellInfo->Targets & TARGET_FLAG_SOURCE_LOCATION)
    {
        targets.SetDst(*bot);
    }
    else
    {
        targets.SetUnitTarget(target);
    }

    if (spellInfo->Effects[0].Effect == SPELL_EFFECT_OPEN_LOCK || spellInfo->Effects[0].Effect == SPELL_EFFECT_SKINNING)
    {
        /*LootObject loot = *aiObjectContext->GetValue<LootObject>("loot target");
        GameObject* go = GetGameObject(loot.guid);
        if (go && go->isSpawned())
        {
            WorldPacket packetgouse(CMSG_GAMEOBJ_USE, 8);
            packetgouse << loot.guid;
            bot->GetSession()->HandleGameObjectUseOpcode(packetgouse);
            targets.SetGOTarget(go);
            faceTo = go;
        }
        else
        {
            if (Unit* creature = GetUnit(loot.guid))
            {
                targets.SetUnitTarget(creature);
                faceTo = creature;
            }
        }*/
    }

    if (bot->isMoving() && spell->GetCastTime())
    {
        // bot->StopMoving();
        SetNextCheckDelay(sPlayerbotAIConfig->reactDelay);
        spell->cancel();
        delete spell;
        return false;
    }

    spell->prepare(&targets);
    SpellCastResult result = spell->CheckCast(false);
    if (result != SPELL_CAST_OK)
    {
        // if (!sPlayerbotAIConfig->logInGroupOnly || (bot->GetGroup() && HasRealPlayerMaster())) {
        TC_LOG_DEBUG("playerbots", "Spell cast failed. - target name: %s, spellid: %u, bot name: %s, result: %u", target->GetName().c_str(), spellId, bot->GetName().c_str(), result);
        // }
        return false;
    }
    
    _aiObjectContext->GetValue<LastSpellCast&>("last spell cast")->Get().Set(spellId, target->GetGUID(), time(nullptr));
    _aiObjectContext->GetValue<PositionMap&>("position")->Get()["random"].Reset();

    if (oldSel)
        bot->SetSelection(oldSel);

    /*if (HasStrategy("debug spell", BOT_STATE_NON_COMBAT))
    {
        std::ostringstream out;
        out << "Casting " << ChatHelper::FormatSpell(spellInfo);
        TellMasterNoFacing(out);
    }*/

    return true;
}

bool PlayerbotAI::CastSpell(uint32 spellId, float x, float y, float z, Item* itemTarget)
{
    if (!spellId)
        return false;

    Pet* pet = bot->GetPet();
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (pet && pet->HasSpell(spellId))
    {
        bool autocast = false;
        for (unsigned int& m_autospell : pet->m_autospells)
        {
            if (m_autospell == spellId)
            {
                autocast = true;
                break;
            }
        }

        pet->ToggleAutocast(spellInfo, !autocast);
        /*std::ostringstream out;
        out << (autocast ? "|cffff0000|Disabling" : "|cFF00ff00|Enabling") << " pet auto-cast for ";
        out << chatHelper.FormatSpell(spellInfo);
        TellMaster(out);*/
        return true;
    }

    // aiObjectContext->GetValue<LastMovement&>("last movement")->Get().Set(nullptr);
    // aiObjectContext->GetValue<time_t>("stay time")->Set(0);

    MotionMaster& mm = *bot->GetMotionMaster();

    if (bot->IsFlying() || bot->HasUnitState(UNIT_STATE_IN_FLIGHT))
        return false;

    // bot->ClearUnitState(UNIT_STATE_CHASE);
    // bot->ClearUnitState(UNIT_STATE_FOLLOW);

    bool failWithDelay = false;
    if (!bot->IsStandState())
    {
        bot->SetStandState(UNIT_STAND_STATE_STAND);
        failWithDelay = true;
    }

    ObjectGuid oldSel = bot->GetSelectedUnit() ? bot->GetSelectedUnit()->GetGUID() : ObjectGuid();

    if (!bot->isMoving())
        bot->SetFacingTo(bot->GetAngle(x, y));

    if (failWithDelay)
    {
        SetNextCheckDelay(sPlayerbotAIConfig->globalCoolDown);
        return false;
    }

    Spell* spell = new Spell(bot, spellInfo, TRIGGERED_NONE);

    SpellCastTargets targets;
    if (spellInfo->Targets & TARGET_FLAG_ITEM)
    {
        /*spell->m_CastItem =
            itemTarget ? itemTarget : aiObjectContext->GetValue<Item*>("item for spell", spellId)->Get();
        targets.SetItemTarget(spell->m_CastItem);

        if (bot->GetTradeData())
        {
            bot->GetTradeData()->SetSpell(spellId);
            delete spell;
            return true;
        }*/
    }
    else if (spellInfo->Targets & TARGET_FLAG_DEST_LOCATION)
    {
        //WorldLocation aoe = aiObjectContext->GetValue<WorldLocation>("aoe position")->Get();
        targets.SetDst(x, y, z, 0.f);
    }
    else if (spellInfo->Targets & TARGET_FLAG_SOURCE_LOCATION)
    {
        targets.SetDst(bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), 0.f);
    }
    else
    {
        return false;
    }

    if (spellInfo->Effects[0].Effect == SPELL_EFFECT_OPEN_LOCK || spellInfo->Effects[0].Effect == SPELL_EFFECT_SKINNING)
    {
        return false;
    }

    spell->prepare(&targets);

    if (bot->isMoving() && spell->GetCastTime())
    {
        // bot->StopMoving();
        SetNextCheckDelay(sPlayerbotAIConfig->reactDelay);
        spell->cancel();
        delete spell;
        return false;
    }

    if (spellInfo->Effects[0].Effect == SPELL_EFFECT_OPEN_LOCK || spellInfo->Effects[0].Effect == SPELL_EFFECT_SKINNING)
    {
        /*LootObject loot = *aiObjectContext->GetValue<LootObject>("loot target");
        if (!loot.IsLootPossible(bot))
        {
            spell->cancel();
            delete spell;
            return false;
        }*/
    }

    _aiObjectContext->GetValue<LastSpellCast&>("last spell cast")->Get().Set(spellId, bot->GetGUID(), time(nullptr));
    _aiObjectContext->GetValue<PositionMap&>("position")->Get()["random"].Reset();

    if (oldSel)
        bot->SetSelection(oldSel);

    /*if (HasStrategy("debug spell", BOT_STATE_NON_COMBAT))
    {
        std::ostringstream out;
        out << "Casting " << ChatHelper::FormatSpell(spellInfo);
        TellMasterNoFacing(out);
    }*/

    return true;
}
