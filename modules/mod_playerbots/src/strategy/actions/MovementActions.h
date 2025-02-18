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

    bool IsMovingAllowed();
    bool IsMovingAllowed(WorldObject* target);

    // -- combat movement
    bool ChaseTo(WorldObject* obj, float distance = 0.0f, float angle = 0.0f);
    bool ReachCombatTo(Unit* target, float distance = 0.0f);

protected:
    void UpdateMovementState();
    void ClearIdleState();

    // -- Generic movement
    void WaitForReach(float distance);
    float MoveDelay(float distance, bool backwards = false);
    bool MoveTo(uint32 mapId, float x, float y, float z, bool idle = false, bool react = false, bool normal_only = false, bool exact_waypoint = false, MovementPriority priority = MovementPriority::MOVEMENT_NORMAL, bool lessDelay = false, bool backwards = false);
    bool MoveTo(WorldObject* target, float distance = 0.0f, MovementPriority priority = MovementPriority::MOVEMENT_NORMAL);
    bool MoveNear(uint32 mapId, float x, float y, float z, float distance = sPlayerbotAIConfig->contactDistance, MovementPriority priority = MovementPriority::MOVEMENT_NORMAL);
    bool MoveNear(WorldObject* target, float distance = sPlayerbotAIConfig->contactDistance, MovementPriority priority = MovementPriority::MOVEMENT_NORMAL);
    bool MoveAway(Unit* target, float distance = sPlayerbotAIConfig->fleeDistance, bool backwards = false);
    bool MoveFromGroup(float distance);
    bool Move(float angle, float distance);
    bool Flee(Unit* target);

    bool IsWaitingForLastMove(MovementPriority priority);
    bool IsMovingAllowed(uint32 mapId, float x, float y, float z);
    bool IsDuplicateMove(uint32 mapId, float x, float y, float z);

    // -- Follow movement
    bool Follow(Unit* target, float distance = sPlayerbotAIConfig->followDistance);
    bool Follow(Unit* target, float distance, float angle);
    float GetFollowAngle();


private:
    const Movement::PointsArray SearchForBestPath(float x, float y, float z, float& modified_z, int maxSearchCount = 5, bool normal_only = false, float step = 8.0f);
};

class MoveRandomAction : public MovementAction
{
public:
    MoveRandomAction(PlayerbotAI* botAI) : MovementAction(botAI, "move random") {}

    bool Execute(Event event) override;
    bool isUseful() override;
    bool isPossible() override;
};

class SetFacingTargetAction : public Action
{
public:
    SetFacingTargetAction(PlayerbotAI* botAI) : Action(botAI, "set facing") {}

    bool Execute(Event event) override;
    bool isUseful() override;
    bool isPossible() override;
};

class MoveFromGroupAction : public MovementAction
{
public:
    MoveFromGroupAction(PlayerbotAI* botAI, std::string const name = "move from group") : MovementAction(botAI, name) {}

    bool Execute(Event event) override;
};

class RunAwayAction : public MovementAction
{
public:
    RunAwayAction(PlayerbotAI* botAI) : MovementAction(botAI, "runaway") {}

    bool Execute(Event event) override;
};

class FleeAction : public MovementAction
{
public:
    FleeAction(PlayerbotAI* botAI, float distance = sPlayerbotAIConfig->spellDistance)
        : MovementAction(botAI, "flee"), distance(distance)
    {
    }

    bool Execute(Event event) override;
    bool isUseful() override;

private:
    float distance;
};