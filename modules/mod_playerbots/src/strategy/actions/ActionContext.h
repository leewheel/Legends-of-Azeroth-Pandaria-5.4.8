/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_ACTIONCONTEXT_H
#define _PLAYERBOT_ACTIONCONTEXT_H

#include "MovementActions.h"
#include "ActionSayHello.h"

class PlayerbotAI;
class ActionContext : public NamedObjectContext<Action>
{
public:
    ActionContext()
    {
        creators["say hello"] = &ActionContext::hello;
        creators["move random"] = &ActionContext::move_random;
    }

private:
    static Action* hello(PlayerbotAI* botAI) { return new SayHelloAction(botAI); }
    static Action* move_random(PlayerbotAI* botAI) { return new MoveRandomAction(botAI); }
};

#endif
