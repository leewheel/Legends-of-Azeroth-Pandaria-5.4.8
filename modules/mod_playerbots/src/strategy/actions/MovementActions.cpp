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
    }
    if (IsWaitingForLastMove(priority))
    {
        return false;
    }*/
    bool generatePath = !bot->IsFlying() && !bot->IsUnderWater() && !bot->IsInWater();
    bool disableMoveSplinePath = sPlayerbotAIConfig->disableMoveSplinePath >= 2 ||
        (sPlayerbotAIConfig->disableMoveSplinePath == 1 && bot->InBattleground());
    if (exact_waypoint || disableMoveSplinePath || !generatePath)
    {
        float distance = bot->GetExactDist(x, y, z);
        if (distance > 0.01f)
        {
            if (bot->IsSitState())
                bot->SetStandState(UNIT_STAND_STATE_STAND);

            // if (bot->IsNonMeleeSpellCast(true))
            // {
            //     bot->CastStop();
            //     botAI->InterruptSpell();
            // }
            MotionMaster& mm = *bot->GetMotionMaster();
            mm.Clear();
            if (!backwards)
            {
                mm.MovePoint(0, x, y, z, generatePath);
            }
            else
            {
                mm.MovePoint(0, x, y, z, generatePath);
                //mm.MovePointBackwards(0, x, y, z, generatePath);
            }
            //float delay = 1000.0f * MoveDelay(distance, backwards);
            //if (lessDelay)
            {
                //delay -= botAI->GetReactDelay();
            }
            //delay = std::max(.0f, delay);
            //delay = std::min((float)sPlayerbotAIConfig->maxWaitForMove, delay);
            //AI_VALUE(LastMovement&, "last movement").Set(mapId, x, y, z, bot->GetOrientation(), 10000.0f, priority);
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

bool MoveRandomAction::isUseful()
{
    return true;
}

bool MoveRandomAction::isPossible()
{
    if (bot->isMoving() || bot->isTurning() || bot->IsFlying() || bot->IsFalling()) return false;

    return true;
}