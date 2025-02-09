#pragma once

#include "Action.h"
#include "PlayerbotAIConfig.h"

class Player;
class PlayerbotAI;
class Unit;
class WorldObject;
class Position;

#define ANGLE_45_DEG (static_cast<float>(M_PI) / 4.f)
#define ANGLE_90_DEG M_PI_2
#define ANGLE_120_DEG (2.f * static_cast<float>(M_PI) / 3.f)

enum class MovementPriority
{
    MOVEMENT_IDLE,
    MOVEMENT_NORMAL,
    MOVEMENT_COMBAT,
    MOVEMENT_FORCED
};

class MovementAction : public Action
{
public:
    MovementAction(PlayerbotAI* botAI, std::string const name);

protected:
    bool MoveTo(uint32 mapId, float x, float y, float z, bool idle = false, bool react = false, bool normal_only = false, bool exact_waypoint = false, MovementPriority priority = MovementPriority::MOVEMENT_NORMAL, bool lessDelay = false, bool backwards = false);
};

class MoveRandomAction : public MovementAction
{
public:
    MoveRandomAction(PlayerbotAI* botAI) : MovementAction(botAI, "move random") {}

    bool Execute(Event event) override;
    bool isUseful() override;
    bool isPossible() override;
};