/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#include "MovementActions.h"

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <string>

#include "Event.h"
#include "PositionValue.h"
#include "G3D/Vector3.h"
#include "GameObject.h"
#include "LastMovementValue.h"
#include "Map.h"
#include "MotionMaster.h"
#include "MoveSplineInitArgs.h"
#include "MovementGenerator.h"
#include "ObjectDefines.h"
#include "ObjectGuid.h"
#include "PathGenerator.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "Position.h"

#include "Random.h"
#include "ServerFacade.h"
#include "SharedDefines.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"

#include "TargetedMovementGenerator.h"
#include "Timer.h"
#include "Transport.h"
#include "Unit.h"
#include "Vehicle.h"
#include "WaypointMovementGenerator.h"
#include "Corpse.h"

MovementAction::MovementAction(PlayerbotAI* botAI, std::string const name) : Action(botAI, name)
{
    bot = botAI->GetBot();
}

bool MovementAction::IsWaitingForLastMove(MovementPriority priority)
{
    LastMovement& lastMove = *context->GetValue<LastMovement&>("last movement");

    if (priority > lastMove.priority)
        return false;

    // heuristic 5s
    if (lastMove.lastdelayTime + lastMove.msTime > getMSTime())
        return true;

    return false;
}

bool MovementAction::ReachCombatTo(Unit* target, float distance)
{
    if (!IsMovingAllowed(target))
        return false;

    float bx = bot->GetPositionX();
    float by = bot->GetPositionY();
    float bz = bot->GetPositionZ();

    float tx = target->GetPositionX();
    float ty = target->GetPositionY();
    float tz = target->GetPositionZ();
    float combatDistance = bot->GetCombatReach() + target->GetCombatReach();
    distance += combatDistance;

    if (bot->GetExactDist(tx, ty, tz) <= distance)
        return false;

    PathGenerator path(bot);
    path.CalculatePath(tx, ty, tz, false);
    PathType type = path.GetPathType();
    int typeOk = PATHFIND_NORMAL | PATHFIND_INCOMPLETE | PATHFIND_SHORTCUT;
    if (!(type & typeOk))
        return false;
    float shortenTo = distance;

    // Avoid walking too far when moving towards each other
    float disToGo = bot->GetExactDist(tx, ty, tz) - distance;
    if (disToGo >= 10.0f)
        shortenTo = disToGo / 2 + distance;

    path.ShortenPathUntilDist(G3D::Vector3(tx, ty, tz), shortenTo);
    G3D::Vector3 endPos = path.GetPath().back();
    return MoveTo(target->GetMapId(), endPos.x, endPos.y, endPos.z, false, false, false, false,
        MovementPriority::MOVEMENT_COMBAT, true);
}

float MovementAction::MoveDelay(float distance, bool backwards)
{
    float speed;
    if (bot->isSwimming())
    {
        speed = backwards ? bot->GetSpeed(MOVE_SWIM_BACK) : bot->GetSpeed(MOVE_SWIM);
    }
    else if (bot->IsFlying())
    {
        speed = backwards ? bot->GetSpeed(MOVE_FLIGHT_BACK) : bot->GetSpeed(MOVE_FLIGHT);
    }
    else
    {
        speed = backwards ? bot->GetSpeed(MOVE_RUN_BACK) : bot->GetSpeed(MOVE_RUN);
    }
    float delay = distance / speed;
    return delay;
}

bool MovementAction::MoveTo(uint32 mapId, float x, float y, float z, bool idle, bool react, bool normal_only,
    bool exact_waypoint, MovementPriority priority, bool lessDelay, bool backwards)
{
    /*UpdateMovementState();
    if (!IsMovingAllowed(mapId, x, y, z))
    {
        return false;
    }
    if (IsDuplicateMove(mapId, x, y, z))
    {
        return false;
    }*/
    if (IsWaitingForLastMove(priority))
    {
        return false;
    }
    bool generatePath = !bot->IsFlying() && !bot->IsUnderWater() && !bot->IsInWater();
    bool disableMoveSplinePath = sPlayerbotAIConfig->disableMoveSplinePath >= 2 ||
        (sPlayerbotAIConfig->disableMoveSplinePath == 1 && bot->InBattleground());
    if (Vehicle* vehicle = bot->GetVehicle())
    {
        VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(bot);
        Unit* vehicleBase = vehicle->GetBase();
        generatePath = vehicleBase->CanFly();
        if (!vehicleBase || !seat || !seat->CanControl())  // is passenger and cant move anyway
            return false;

        float distance = vehicleBase->GetExactDist(x, y, z);  // use vehicle distance, not bot
        if (distance > 0.01f)
        {
            MotionMaster& mm = *vehicleBase->GetMotionMaster();  // need to move vehicle, not bot
            mm.Clear();
            if (!backwards)
            {
                mm.MovePoint(0, x, y, z, generatePath);
            }
            else
            {
                mm.MovePointBackwards(0, x, y, z, generatePath);
            }
            float speed = backwards ? vehicleBase->GetSpeed(MOVE_RUN_BACK) : vehicleBase->GetSpeed(MOVE_RUN);
            float delay = 1000.0f * (distance / speed);
            if (lessDelay)
            {
                delay -= botAI->GetReactDelay();
            }
            delay = std::max(.0f, delay);
            delay = std::min((float)sPlayerbotAIConfig->maxWaitForMove, delay);
            AI_VALUE(LastMovement&, "last movement").Set(mapId, x, y, z, bot->GetOrientation(), delay, priority);
            return true;
        }
    }
    else if (exact_waypoint || disableMoveSplinePath || !generatePath)
    {
        float distance = bot->GetExactDist(x, y, z);
        if (distance > 0.01f)
        {
            if (bot->IsSitState())
                bot->SetStandState(UNIT_STAND_STATE_STAND);

            MotionMaster& mm = *bot->GetMotionMaster();
            mm.Clear();
            if (!backwards)
            {
                mm.MovePoint(0, x, y, z, generatePath);
            }
            else
            {
                mm.MovePointBackwards(0, x, y, z, generatePath);
            }
            float delay = 1000.0f * MoveDelay(distance, backwards);
            if (lessDelay)
            {
                delay -= botAI->GetReactDelay();
            }
            delay = std::max(.0f, delay);
            delay = std::min((float)sPlayerbotAIConfig->maxWaitForMove, delay);
            AI_VALUE(LastMovement&, "last movement").Set(mapId, x, y, z, bot->GetOrientation(), delay, priority);
            return true;
        }
    }
    else
    {
        float modifiedZ;
        Movement::PointsArray path = SearchForBestPath(x, y, z, modifiedZ, sPlayerbotAIConfig->maxMovementSearchTime, normal_only);
        if (modifiedZ == INVALID_HEIGHT)
        {
            return false;
        }
        float distance = bot->GetExactDist(x, y, modifiedZ);
        if (distance > 0.01f)
        {
            if (bot->IsSitState())
                bot->SetStandState(UNIT_STAND_STATE_STAND);

            MotionMaster& mm = *bot->GetMotionMaster();
            G3D::Vector3 endP = path.back();
            mm.Clear();
            if (!backwards)
            {
                mm.MovePoint(0, x, y, z, generatePath);
            }
            else
            {
                mm.MovePointBackwards(0, x, y, z, generatePath);
            }
             float delay = 1000.0f * MoveDelay(distance, backwards);
            if (lessDelay)
            {
                delay -= botAI->GetReactDelay();
            }
            delay = std::max(.0f, delay);
            delay = std::min((float)sPlayerbotAIConfig->maxWaitForMove, delay);
            AI_VALUE(LastMovement&, "last movement").Set(mapId, x, y, z, bot->GetOrientation(), delay, priority);
            return true;
        }
    }
    return false;
}

bool MoveRandomAction::Execute(Event event)
{
    float distance = sPlayerbotAIConfig->tooCloseDistance + urand(10, 30);
    const float x = bot->GetPositionX();
    const float y = bot->GetPositionY();
    const float z = bot->GetPositionZ();
    int attempts = 5;
    Map* map = bot->GetMap();
    while (--attempts)
    {
        float angle = (float)rand_norm() * 2 * static_cast<float>(M_PI);
        float dx = x + distance * cos(angle);
        float dy = y + distance * sin(angle);
        float dz = z;
        if (!map->CheckCollisionAndGetValidCoords(bot, bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(),
            dx, dy, dz))
            continue;

        if (map->IsInWater(bot->GetPhaseMask(), dx, dy, dz))
            continue;

        bool moved = MoveTo(bot->GetMapId(), dx, dy, dz, false, false, false, true);
        if (moved)
            return true;
    }

    return false;
}

bool MovementAction::MoveNear(uint32 mapId, float x, float y, float z, float distance, MovementPriority priority)
{
    float angle = GetFollowAngle();
    return MoveTo(mapId, x + cos(angle) * distance, y + sin(angle) * distance, z, false, false, false, false, priority);
}

bool MovementAction::MoveNear(WorldObject* target, float distance, MovementPriority priority)
{
    if (!target)
        return false;

    distance += target->GetCombatReach();

    float x = target->GetPositionX();
    float y = target->GetPositionY();
    float z = target->GetPositionZ();
    float followAngle = GetFollowAngle();

    for (float angle = followAngle; angle <= followAngle + static_cast<float>(2 * M_PI);
        angle += static_cast<float>(M_PI / 4.f))
    {
        float x = target->GetPositionX() + cos(angle) * distance;
        float y = target->GetPositionY() + sin(angle) * distance;
        float z = target->GetPositionZ();

        if (!bot->IsWithinLOS(x, y, z))
            continue;

        bool moved = MoveTo(target->GetMapId(), x, y, z, false, false, false, false, priority);
        if (moved)
            return true;
    }

    // botAI->TellError("All paths not in LOS");
    return false;
}

float MovementAction::GetFollowAngle()
{
    Player* master = GetMaster();
    Group* group = master ? master->GetGroup() : bot->GetGroup();
    if (!group)
        return 0.0f;

    uint32 index = 1;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        if (ref->GetSource() == master)
            continue;

        if (ref->GetSource() == bot)
            return 2 * M_PI / (group->GetMembersCount() - 1) * index;

        ++index;
    }

    return 0;
}

bool MoveRandomAction::isUseful()
{
    return true;
}

bool MovementAction::IsMovingAllowed(WorldObject* target)
{
    if (!target)
        return false;

    if (bot->GetMapId() != target->GetMapId())
        return false;

    return IsMovingAllowed();
}

bool MovementAction::IsMovingAllowed()
{
    // do not allow if not vehicle driver
    if (botAI->IsInVehicle() && !botAI->IsInVehicle(true))
        return false;

    if (bot->isFrozen() || bot->IsPolymorphed() || (bot->isDead() && !bot->HasPlayerFlag(PLAYER_FLAGS_GHOST)) ||
        bot->IsBeingTeleported() || bot->HasRootAura() || bot->HasSpiritOfRedemptionAura() ||
        bot->HasConfuseAura() || bot->IsCharmed() || bot->HasStunAura() ||
        bot->IsInFlight() || bot->HasUnitState(UNIT_STATE_LOST_CONTROL))
        return false;

    if (bot->GetMotionMaster()->GetMotionSlotType(MOTION_SLOT_CONTROLLED) != NULL_MOTION_TYPE)
    {
        return false;
    }

    return bot->GetMotionMaster()->GetCurrentMovementGeneratorType() != FLIGHT_MOTION_TYPE;
}

bool MoveRandomAction::isPossible()
{
    if (bot->IsInCombat() ||
        !AI_VALUE(bool, "can move around") ||
        !bot->CanFreeMove() ||
        !botAI->CanMove()) return false;

    return true;
}

const Movement::PointsArray MovementAction::SearchForBestPath(float x, float y, float z, float& modified_z,
    int maxSearchCount, bool normal_only, float step)
{
    bool found = false;
    modified_z = INVALID_HEIGHT;
    float tempZ = bot->GetMapHeight(x, y, z);
    PathGenerator gen(bot);
    gen.CalculatePath(x, y, tempZ);
    Movement::PointsArray result = gen.GetPath();
    float min_length = gen.getPathLength();
    int typeOk = PATHFIND_NORMAL | PATHFIND_INCOMPLETE;
    if ((gen.GetPathType() & typeOk) && abs(tempZ - z) < 0.5f)
    {
        modified_z = tempZ;
        return result;
    }
    // Start searching
    if (gen.GetPathType() & typeOk)
    {
        modified_z = tempZ;
        found = true;
    }
    int count = 1;
    for (float delta = step; count < maxSearchCount / 2 + 1; count++, delta += step)
    {
        tempZ = bot->GetMapHeight(x, y, z + delta);
        if (tempZ == INVALID_HEIGHT)
        {
            continue;
        }
        PathGenerator gen(bot);
        gen.CalculatePath(x, y, tempZ);
        if ((gen.GetPathType() & typeOk) && gen.getPathLength() < min_length)
        {
            found = true;
            min_length = gen.getPathLength();
            result = gen.GetPath();
            modified_z = tempZ;
        }
    }
    for (float delta = -step; count < maxSearchCount; count++, delta -= step)
    {
        tempZ = bot->GetMapHeight(x, y, z + delta);
        if (tempZ == INVALID_HEIGHT)
        {
            continue;
        }
        PathGenerator gen(bot);
        gen.CalculatePath(x, y, tempZ);
        if ((gen.GetPathType() & typeOk) && gen.getPathLength() < min_length)
        {
            found = true;
            min_length = gen.getPathLength();
            result = gen.GetPath();
            modified_z = tempZ;
        }
    }
    if (!found && normal_only)
    {
        modified_z = INVALID_HEIGHT;
        return Movement::PointsArray{};
    }
    if (!found && !normal_only)
    {
        return result;
    }
    return result;
}

bool SetFacingTargetAction::Execute(Event event)
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
        return false;

    if (bot->HasUnitState(UNIT_STATE_IN_FLIGHT))
        return true;

    sServerFacade->SetFacingTo(bot, target);
    botAI->SetNextCheckDelay(sPlayerbotAIConfig->reactDelay);
    return true;
}

bool SetFacingTargetAction::isUseful()
{
    return !AI_VALUE2(bool, "facing", "current target");
}

bool SetFacingTargetAction::isPossible()
{
    if (bot->isFrozen() || bot->IsPolymorphed() || (bot->isDead() && !bot->HasPlayerFlag(PLAYER_FLAGS_GHOST)) ||
        bot->IsBeingTeleported() || bot->HasConfuseAura() || bot->IsCharmed() ||
        bot->HasStunAura() || bot->IsInFlight() ||
        bot->HasUnitState(UNIT_STATE_LOST_CONTROL))
        return false;

    return true;
}