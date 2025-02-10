/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_TRIGGERCONTEXT_H
#define _PLAYERBOT_TRIGGERCONTEXT_H


#include "NamedObjectContext.h"
#include "GenericTriggers.h"
#include "HealthTrigger.h"
#include "RangeTriggers.h"

class PlayerbotAI;
class TriggerContext : public NamedObjectContext<Trigger>
{
public:
    TriggerContext()
    {
        creators["timer"] = &TriggerContext::Timer;
        creators["random"] = &TriggerContext::Random;
        creators["seldom"] = &TriggerContext::seldom;
        creators["often"] = &TriggerContext::often;
        creators["always trigger"] = &TriggerContext::always;

        creators["dead"] = &TriggerContext::Dead;
        creators["corpse near"] = &TriggerContext::corpse_near;

        creators["enemy out of melee"] = &TriggerContext::EnemyOutOfMelee;
        creators["enemy out of spell"] = &TriggerContext::EnemyOutOfSpell;
        creators["enemy too close for spell"] = &TriggerContext::enemy_too_close_for_spell;
        creators["enemy too close for shoot"] = &TriggerContext::enemy_too_close_for_shoot;
        creators["enemy too close for auto shot"] = &TriggerContext::enemy_too_close_for_auto_shot;
        creators["enemy too close for melee"] = &TriggerContext::enemy_too_close_for_melee;
        creators["enemy is close"] = &TriggerContext::enemy_is_close;
        creators["enemy within melee"] = &TriggerContext::enemy_within_melee;
        creators["party member to heal out of spell range"] = &TriggerContext::party_member_to_heal_out_of_spell_range;
        creators["no target"] = &TriggerContext::NoTarget;
        creators["target changed"] = &TriggerContext::target_changed;
        creators["invalid target"] = &TriggerContext::invalid_target;
    }

private:
    static Trigger* Timer(PlayerbotAI* botAI) { return new TimerTrigger(botAI); }
    static Trigger* Random(PlayerbotAI* botAI) { return new RandomTrigger(botAI, "random", 20); }
    static Trigger* seldom(PlayerbotAI* botAI) { return new RandomTrigger(botAI, "seldom", 300); }
    static Trigger* often(PlayerbotAI* botAI) { return new RandomTrigger(botAI, "often", 5); }
    static Trigger* always(PlayerbotAI* botAI) { return new AlwaysTrigger(botAI); }

    static Trigger* Dead(PlayerbotAI* botAI) { return new DeadTrigger(botAI); }
    static Trigger* corpse_near(PlayerbotAI* botAI) { return new CorpseNearTrigger(botAI); }

    static Trigger* EnemyOutOfMelee(PlayerbotAI* botAI) { return new EnemyOutOfMeleeTrigger(botAI); }
    static Trigger* EnemyOutOfSpell(PlayerbotAI* botAI) { return new EnemyOutOfSpellRangeTrigger(botAI); }
    static Trigger* enemy_too_close_for_spell(PlayerbotAI* botAI) { return new EnemyTooCloseForSpellTrigger(botAI); }
    static Trigger* enemy_too_close_for_auto_shot(PlayerbotAI* botAI)
    {
        return new EnemyTooCloseForAutoShotTrigger(botAI);
    }
    static Trigger* enemy_too_close_for_shoot(PlayerbotAI* botAI) { return new EnemyTooCloseForShootTrigger(botAI); }
    static Trigger* enemy_too_close_for_melee(PlayerbotAI* botAI) { return new EnemyTooCloseForMeleeTrigger(botAI); }
    static Trigger* enemy_is_close(PlayerbotAI* botAI) { return new EnemyIsCloseTrigger(botAI); }
    static Trigger* enemy_within_melee(PlayerbotAI* botAI) { return new EnemyWithinMeleeTrigger(botAI); }
    static Trigger* party_member_to_heal_out_of_spell_range(PlayerbotAI* botAI)
    {
        return new PartyMemberToHealOutOfSpellRangeTrigger(botAI);
    }
    static Trigger* target_changed(PlayerbotAI* botAI) { return new TargetChangedTrigger(botAI); }
    static Trigger* NoTarget(PlayerbotAI* botAI) { return new NoTargetTrigger(botAI); }
    static Trigger* invalid_target(PlayerbotAI* botAI) { return new InvalidTargetTrigger(botAI); }
};

#endif
