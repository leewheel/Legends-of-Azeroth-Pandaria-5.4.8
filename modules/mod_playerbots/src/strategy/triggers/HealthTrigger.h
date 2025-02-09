#ifndef _PLAYERBOT_HEALTHTRIGGERS_H
#define _PLAYERBOT_HEALTHTRIGGERS_H

#include <stdexcept>

#include "PlayerbotAIConfig.h"
#include "Trigger.h"

class PlayerbotAI; 

class DeadTrigger : public Trigger
{
public:
    DeadTrigger(PlayerbotAI* botAI) : Trigger(botAI, "dead") {}

    std::string const GetTargetName() override { return "self target"; }
    bool IsActive() override;
};

#endif