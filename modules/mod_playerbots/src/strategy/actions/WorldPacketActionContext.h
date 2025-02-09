/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_WORLDPACKETACTIONCONTEXT_H
#define _PLAYERBOT_WORLDPACKETACTIONCONTEXT_H


#include "NamedObjectContext.h"

#include "AcceptInvitationAction.h"
#include "LeaveGroupAction.h"

class PlayerbotAI;

class WorldPacketActionContext : public NamedObjectContext<Action>
{
public:
    WorldPacketActionContext()
    {
        // party handlers
        creators["accept invitation"] = &WorldPacketActionContext::accept_invitation;
        creators["uninvite"] = &WorldPacketActionContext::uninvite;
    }

private:
    static Action* accept_invitation(PlayerbotAI* botAI) { return new AcceptInvitationAction(botAI); }
    static Action* uninvite(PlayerbotAI* botAI) { return new UninviteAction(botAI); }
};

#endif
