/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_WORLDPACKETTRIGGERCONTEXT_H
#define _PLAYERBOT_WORLDPACKETTRIGGERCONTEXT_H

#include "NamedObjectContext.h"
#include "WorldPacketTrigger.h"

class WorldPacketTriggerContext : public NamedObjectContext<Trigger>
{
public:
    WorldPacketTriggerContext()
    {
        creators["group invite"] = &WorldPacketTriggerContext::group_invite;
        creators["uninvite"] = &WorldPacketTriggerContext::uninvite;
        creators["uninvite guid"] = &WorldPacketTriggerContext::uninvite_guid;
    }

private:
    static Trigger* group_invite(PlayerbotAI* botAI) { return new WorldPacketTrigger(botAI, "group invite"); }
    static Trigger* uninvite(PlayerbotAI* botAI) { return new WorldPacketTrigger(botAI, "uninvite"); }
    static Trigger* uninvite_guid(PlayerbotAI* botAI) { return new WorldPacketTrigger(botAI, "uninvite guid"); }
};

#endif
