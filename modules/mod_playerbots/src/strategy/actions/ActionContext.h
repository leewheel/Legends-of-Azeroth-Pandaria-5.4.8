/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_ACTIONCONTEXT_H
#define _PLAYERBOT_ACTIONCONTEXT_H

#include "AttackActions.h"
#include "ActionSayHello.h"
#include "ChooseTargetActions.h"
#include "GenericActions.h"
#include "MovementActions.h"
#include "ReachTargetActions.h"

class PlayerbotAI;
class ActionContext : public NamedObjectContext<Action>
{
public:
    ActionContext()
    {
        creators["say hello"] = &ActionContext::hello;
        creators["move random"] = &ActionContext::move_random;

        creators["set facing"] = &ActionContext::set_facing;
        creators["drop target"] = &ActionContext::drop_target;
        creators["attack"] = &ActionContext::melee;
        creators["melee"] = &ActionContext::melee;
        creators["attack anything"] = &ActionContext::attack_anything;

        creators["reach spell"] = &ActionContext::ReachSpell;
        creators["reach melee"] = &ActionContext::ReachMelee;
        creators["reach party member to heal"] = &ActionContext::reach_party_member_to_heal;
        creators["reach party member to resurrect"] = &ActionContext::reach_party_member_to_resurrect;
    }

private:
    static Action* hello(PlayerbotAI* botAI) { return new SayHelloAction(botAI); }
    static Action* move_random(PlayerbotAI* botAI) { return new MoveRandomAction(botAI); }

    static Action* set_facing(PlayerbotAI* botAI) { return new SetFacingTargetAction(botAI); }
    static Action* drop_target(PlayerbotAI* botAI) { return new DropTargetAction(botAI); }
    static Action* melee(PlayerbotAI* botAI) { return new MeleeAction(botAI); }
    static Action* attack_anything(PlayerbotAI* botAI) { return new AttackAnythingAction(botAI); }

    static Action* ReachSpell(PlayerbotAI* botAI) { return new ReachSpellAction(botAI); }
    static Action* ReachMelee(PlayerbotAI* botAI) { return new ReachMeleeAction(botAI); }
    static Action* reach_party_member_to_heal(PlayerbotAI* botAI) { return new ReachPartyMemberToHealAction(botAI); }
    static Action* reach_party_member_to_resurrect(PlayerbotAI* botAI) { return new ReachPartyMemberToResurrectAction(botAI); }
};

#endif
