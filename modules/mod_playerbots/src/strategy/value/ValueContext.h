#ifndef _PLAYERBOT_VALUECONTEXT_H
#define _PLAYERBOT_VALUECONTEXT_H

#include "PositionValue.h"
#include "LastMovementValue.h"
#include "StatsValues.h"
#include "AttackersValue.h"
#include "SelfTargetValue.h"
#include "PossibleTargetsValue.h"
#include "CurrentTargetValue.h"
#include "NearestUnitsValue.h"

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
        creators["group"] = &ValueContext::group;
        creators["death count"] = &ValueContext::death_count;
        creators["experience"] = &ValueContext::experience;
        creators["disperse distance"] = &ValueContext::disperse_distance;
        creators["last flee angle"] = &ValueContext::last_flee_angle;
        creators["last flee timestamp"] = &ValueContext::last_flee_timestamp;
        creators["recently flee info"] = &ValueContext::recently_flee_info;

        creators["attackers"] = &ValueContext::attackers;
        creators["possible adds"] = &ValueContext::possible_adds;
        creators["prioritized targets"] = &ValueContext::prioritized_targets;

        creators["possible triggers"] = &ValueContext::possible_triggers;
        creators["possible targets"] = &ValueContext::possible_targets;
        creators["possible targets no los"] = &ValueContext::possible_targets_no_los;

        creators["position"] = &ValueContext::position;
        creators["current position"] = &ValueContext::current_position;

        creators["all targets"] = &ValueContext::all_targets;
        creators["self target"] = &ValueContext::self_target;
        creators["current target"] = &ValueContext::current_target;
    }

private:
    
    static UntypedValue* group(PlayerbotAI* botAI) { return new IsInGroupValue(botAI); }
    static UntypedValue* last_movement(PlayerbotAI* botAI) { return new LastMovementValue(botAI); }
    static UntypedValue* stay_time(PlayerbotAI* botAI) { return new StayTimeValue(botAI); }
    static UntypedValue* mounted(PlayerbotAI* botAI) { return new IsMountedValue(botAI); }
    static UntypedValue* health(PlayerbotAI* botAI) { return new HealthValue(botAI); }
    static UntypedValue* rage(PlayerbotAI* botAI) { return new RageValue(botAI); }
    static UntypedValue* energy(PlayerbotAI* botAI) { return new EnergyValue(botAI); }
    static UntypedValue* mana(PlayerbotAI* botAI) { return new ManaValue(botAI); }
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
    
    static UntypedValue* attackers(PlayerbotAI* botAI) { return new AttackersValue(botAI); }
    static UntypedValue* possible_adds(PlayerbotAI* botAI) { return new PossibleAddsValue(botAI); }
    static UntypedValue* prioritized_targets(PlayerbotAI* botAI) { return new PrioritizedTargetsValue(botAI); }

    static UntypedValue* all_targets(PlayerbotAI* botAI) { return new AllTargetsValue(botAI); }
    static UntypedValue* self_target(PlayerbotAI* botAI) { return new SelfTargetValue(botAI); }
    static UntypedValue* current_target(PlayerbotAI* botAI) { return new CurrentTargetValue(botAI); }
    static UntypedValue* possible_triggers(PlayerbotAI* botAI) { return new PossibleTriggersValue(botAI); }
    static UntypedValue* possible_targets(PlayerbotAI* botAI) { return new PossibleTargetsValue(botAI); }
    static UntypedValue* possible_targets_no_los(PlayerbotAI* botAI) { return new PossibleTargetsValue(botAI, "possible targets", sPlayerbotAIConfig->sightDistance, true); }
};

#endif
