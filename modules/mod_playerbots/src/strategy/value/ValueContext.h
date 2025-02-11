#ifndef _PLAYERBOT_VALUECONTEXT_H
#define _PLAYERBOT_VALUECONTEXT_H

#include "AttackersValue.h"
#include "CurrentTargetValue.h"
#include "DistanceValue.h"
#include "EnemyPlayerValue.h"
#include "GroupValues.h"
#include "GrindTargetValue.h"
#include "InvalidTargetValue.h"
#include "IsBehindValue.h"
#include "IsMovingValue.h"
#include "LastMovementValue.h"
#include "LastSpellCastValue.h"
#include "LastSpellCastTimeValue.h"
#include "MaintenanceValues.h"
#include "ManaSaveLevelValue.h"
#include "NearestUnitsValue.h"
#include "NearestFriendlyPlayersValue.h"
#include "PositionValue.h"
#include "StatsValues.h"
#include "SelfTargetValue.h"
#include "SpellCastUsefulValue.h"
#include "SpellIdValue.h"
#include "PossibleTargetsValue.h"
#include "RangeValues.h"

class PlayerbotAI;
class ValueContext : public NamedObjectContext<UntypedValue>
{
public:
    ValueContext()
    {
        creators["health"] = &ValueContext::health;
        creators["rage"] = &ValueContext::rage;
        creators["energy"] = &ValueContext::energy;
        creators["mana"] = &ValueContext::mana;
        creators["mana save level"] = &ValueContext::mana_save_level;
        creators["combo"] = &ValueContext::combo;
        creators["dead"] = &ValueContext::dead;
        creators["pet dead"] = &ValueContext::pet_dead;
        creators["has mana"] = &ValueContext::has_mana;
        creators["mounted"] = &ValueContext::mounted;
        creators["last movement"] = &ValueContext::last_movement;
        creators["stay time"] = &ValueContext::stay_time;
        creators["last taxi"] = &ValueContext::last_movement;
        creators["last area trigger"] = &ValueContext::last_movement;
        creators["combat"] = &ValueContext::combat;
        creators["bag space"] = &ValueContext::bag_space;
        creators["durability"] = &ValueContext::durability;
        creators["speed"] = &ValueContext::speed;
        creators["death count"] = &ValueContext::death_count;
        creators["experience"] = &ValueContext::experience;
        creators["disperse distance"] = &ValueContext::disperse_distance;
        creators["last flee angle"] = &ValueContext::last_flee_angle;
        creators["last flee timestamp"] = &ValueContext::last_flee_timestamp;
        creators["recently flee info"] = &ValueContext::recently_flee_info;

        creators["distance"] = &ValueContext::distance;
        creators["moving"] = &ValueContext::moving;
        creators["swimming"] = &ValueContext::swimming;
        creators["behind"] = &ValueContext::behind;
        creators["facing"] = &ValueContext::facing;
        creators["attackers"] = &ValueContext::attackers;
        creators["possible adds"] = &ValueContext::possible_adds;
        creators["prioritized targets"] = &ValueContext::prioritized_targets;

        creators["possible triggers"] = &ValueContext::possible_triggers;
        creators["possible targets"] = &ValueContext::possible_targets;
        creators["possible targets no los"] = &ValueContext::possible_targets_no_los;

        creators["position"] = &ValueContext::position;
        creators["current position"] = &ValueContext::current_position;
        creators["can move around"] = &ValueContext::can_move_around;

        creators["all targets"] = &ValueContext::all_targets;
        creators["self target"] = &ValueContext::self_target;
        creators["current target"] = &ValueContext::current_target;
        creators["old target"] = &ValueContext::old_target;
        creators["grind target"] = &ValueContext::grind_target;
        creators["pull target"] = &ValueContext::pull_target;
        creators["invalid target"] = &ValueContext::invalid_target;
        creators["nearest friendly players"] = &ValueContext::nearest_friendly_players;
        creators["closest friendly players"] = &ValueContext::closest_friendly_players;
        creators["nearest enemy players"] = &ValueContext::nearest_enemy_players;
        creators["enemy player target"] = &ValueContext::enemy_player_target;

        creators["can move around"] = &ValueContext::can_move_around;
        creators["should home bind"] = &ValueContext::should_home_bind;
        creators["should repair"] = &ValueContext::should_repair;
        creators["can repair"] = &ValueContext::can_repair;
        creators["should sell"] = &ValueContext::should_sell;
        creators["can sell"] = &ValueContext::can_sell;
        creators["can fight equal"] = &ValueContext::can_fight_equal;
        creators["can fight elite"] = &ValueContext::can_fight_elite;
        creators["can fight boss"] = &ValueContext::can_fight_boss;

        creators["range"] = &ValueContext::range;
        creators["group"] = &ValueContext::group;
        creators["group members"] = &ValueContext::group_members;
        creators["following party"] = &ValueContext::following_party;
        creators["near leader"] = &ValueContext::near_leader;
        creators["and"] = &ValueContext::and_value;
        creators["group count"] = &ValueContext::group_count;
        creators["group and"] = &ValueContext::group_and;
        creators["group or"] = &ValueContext::group_or;
        creators["group ready"] = &ValueContext::group_ready;

        creators["spell id"] = &ValueContext::spell_id;
        creators["vehicle spell id"] = &ValueContext::vehicle_spell_id;
        //creators["item for spell"] = &ValueContext::item_for_spell;
        creators["spell cast useful"] = &ValueContext::spell_cast_useful;
        creators["last spell cast"] = &ValueContext::last_spell_cast;
        creators["last spell cast time"] = &ValueContext::last_spell_cast_time;
    }

private:
    static UntypedValue* last_movement(PlayerbotAI* botAI) { return new LastMovementValue(botAI); }
    static UntypedValue* stay_time(PlayerbotAI* botAI) { return new StayTimeValue(botAI); }
    static UntypedValue* mounted(PlayerbotAI* botAI) { return new IsMountedValue(botAI); }
    static UntypedValue* health(PlayerbotAI* botAI) { return new HealthValue(botAI); }
    static UntypedValue* rage(PlayerbotAI* botAI) { return new RageValue(botAI); }
    static UntypedValue* energy(PlayerbotAI* botAI) { return new EnergyValue(botAI); }
    static UntypedValue* mana(PlayerbotAI* botAI) { return new ManaValue(botAI); }
    static UntypedValue* mana_save_level(PlayerbotAI* botAI) { return new ManaSaveLevelValue(botAI); }
    static UntypedValue* combo(PlayerbotAI* botAI) { return new ComboPointsValue(botAI); }
    static UntypedValue* dead(PlayerbotAI* botAI) { return new IsDeadValue(botAI); }
    static UntypedValue* pet_dead(PlayerbotAI* botAI) { return new PetIsDeadValue(botAI); }
    static UntypedValue* has_mana(PlayerbotAI* botAI) { return new HasManaValue(botAI); }   
    static UntypedValue* combat(PlayerbotAI* botAI) { return new IsInCombatValue(botAI); }
    static UntypedValue* bag_space(PlayerbotAI* botAI) { return new BagSpaceValue(botAI); }
    static UntypedValue* durability(PlayerbotAI* botAI) { return new DurabilityValue(botAI); }
    static UntypedValue* speed(PlayerbotAI* botAI) { return new SpeedValue(botAI); }
    static UntypedValue* death_count(PlayerbotAI* botAI) { return new DeathCountValue(botAI); }
    static UntypedValue* experience(PlayerbotAI* botAI) { return new ExperienceValue(botAI); }
    static UntypedValue* disperse_distance(PlayerbotAI* ai) { return new DisperseDistanceValue(ai); }
    static UntypedValue* last_flee_angle(PlayerbotAI* ai) { return new LastFleeAngleValue(ai); }
    static UntypedValue* last_flee_timestamp(PlayerbotAI* ai) { return new LastFleeTimestampValue(ai); }
    static UntypedValue* recently_flee_info(PlayerbotAI* ai) { return new RecentlyFleeInfo(ai); }
    static UntypedValue* position(PlayerbotAI* botAI) { return new PositionValue(botAI); }
    static UntypedValue* current_position(PlayerbotAI* botAI) { return new CurrentPositionValue(botAI); }
    static UntypedValue* behind(PlayerbotAI* botAI) { return new IsBehindValue(botAI); }
    static UntypedValue* facing(PlayerbotAI* botAI) { return new IsFacingValue(botAI); }
    static UntypedValue* moving(PlayerbotAI* botAI) { return new IsMovingValue(botAI); }
    static UntypedValue* swimming(PlayerbotAI* botAI) { return new IsSwimmingValue(botAI); }
    static UntypedValue* distance(PlayerbotAI* botAI) { return new DistanceValue(botAI); }

    static UntypedValue* attackers(PlayerbotAI* botAI) { return new AttackersValue(botAI); }
    static UntypedValue* possible_adds(PlayerbotAI* botAI) { return new PossibleAddsValue(botAI); }
    static UntypedValue* prioritized_targets(PlayerbotAI* botAI) { return new PrioritizedTargetsValue(botAI); }

    static UntypedValue* all_targets(PlayerbotAI* botAI) { return new AllTargetsValue(botAI); }
    static UntypedValue* self_target(PlayerbotAI* botAI) { return new SelfTargetValue(botAI); }
    static UntypedValue* current_target(PlayerbotAI* botAI) { return new CurrentTargetValue(botAI); }
    static UntypedValue* possible_triggers(PlayerbotAI* botAI) { return new PossibleTriggersValue(botAI); }
    static UntypedValue* possible_targets(PlayerbotAI* botAI) { return new PossibleTargetsValue(botAI); }
    static UntypedValue* possible_targets_no_los(PlayerbotAI* botAI) { return new PossibleTargetsValue(botAI, "possible targets", sPlayerbotAIConfig->sightDistance, true); }
    static UntypedValue* old_target(PlayerbotAI* botAI) { return new CurrentTargetValue(botAI); }
    static UntypedValue* grind_target(PlayerbotAI* botAI) { return new GrindTargetValue(botAI); }
    static UntypedValue* pull_target(PlayerbotAI* botAI) { return new PullTargetValue(botAI); }
    static UntypedValue* invalid_target(PlayerbotAI* botAI) { return new InvalidTargetValue(botAI); }
    static UntypedValue* nearest_friendly_players(PlayerbotAI* botAI) { return new NearestFriendlyPlayersValue(botAI); }
    static UntypedValue* closest_friendly_players(PlayerbotAI* botAI) { return new NearestFriendlyPlayersValue(botAI, INTERACTION_DISTANCE); }
    static UntypedValue* nearest_enemy_players(PlayerbotAI* botAI) { return new NearestEnemyPlayersValue(botAI); }
    static UntypedValue* enemy_player_target(PlayerbotAI* botAI) { return new EnemyPlayerValue(botAI); }

    static UntypedValue* can_move_around(PlayerbotAI* botAI) { return new CanMoveAroundValue(botAI); }
    static UntypedValue* should_home_bind(PlayerbotAI* botAI) { return new ShouldHomeBindValue(botAI); }
    static UntypedValue* should_repair(PlayerbotAI* botAI) { return new ShouldRepairValue(botAI); }
    static UntypedValue* can_repair(PlayerbotAI* botAI) { return new CanRepairValue(botAI); }
    static UntypedValue* should_sell(PlayerbotAI* botAI) { return new ShouldSellValue(botAI); }
    static UntypedValue* can_sell(PlayerbotAI* botAI) { return new CanSellValue(botAI); }
    static UntypedValue* can_fight_equal(PlayerbotAI* botAI) { return new CanFightEqualValue(botAI); }
    static UntypedValue* can_fight_elite(PlayerbotAI* botAI) { return new CanFightEliteValue(botAI); }
    static UntypedValue* can_fight_boss(PlayerbotAI* botAI) { return new CanFightBossValue(botAI); }

    static UntypedValue* range(PlayerbotAI* botAI) { return new RangeValue(botAI); }
    static UntypedValue* group_members(PlayerbotAI* botAI) { return new GroupMembersValue(botAI); }
    static UntypedValue* following_party(PlayerbotAI* botAI) { return new IsFollowingPartyValue(botAI); }
    static UntypedValue* near_leader(PlayerbotAI* botAI) { return new IsNearLeaderValue(botAI); }
    static UntypedValue* group(PlayerbotAI* botAI) { return new IsInGroupValue(botAI); }
    static UntypedValue* and_value(PlayerbotAI* botAI) { return new BoolANDValue(botAI); }
    static UntypedValue* group_count(PlayerbotAI* botAI) { return new GroupBoolCountValue(botAI); }
    static UntypedValue* group_and(PlayerbotAI* botAI) { return new GroupBoolANDValue(botAI); }
    static UntypedValue* group_or(PlayerbotAI* botAI) { return new GroupBoolORValue(botAI); }
    static UntypedValue* group_ready(PlayerbotAI* botAI) { return new GroupReadyValue(botAI); }

    static UntypedValue* spell_id(PlayerbotAI* botAI) { return new SpellIdValue(botAI); }
    static UntypedValue* vehicle_spell_id(PlayerbotAI* botAI) { return new VehicleSpellIdValue(botAI); }
    //static UntypedValue* inventory_item(PlayerbotAI* botAI) { return new InventoryItemValue(botAI); }
    static UntypedValue* last_spell_cast(PlayerbotAI* botAI) { return new LastSpellCastValue(botAI); }
    static UntypedValue* last_spell_cast_time(PlayerbotAI* botAI) { return new LastSpellCastTimeValue(botAI); }
    static UntypedValue* spell_cast_useful(PlayerbotAI* botAI) { return new SpellCastUsefulValue(botAI); }
};

#endif
