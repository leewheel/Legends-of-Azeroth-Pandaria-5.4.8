/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#include "ArmsWarriorStrategy.h"

#include "Playerbots.h"

class ArmsWarriorStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    ArmsWarriorStrategyActionNodeFactory()
    {
        creators["mortal strike"] = &mortal_strike;
        creators["slam"] = &slam;

        creators["overpower"] = &overpower;
        creators["die by the sword"] = &die_by_the_sword;
        creators["sweeping strikes"] = &sweeping_strikes;
        creators["colossus smash"] = &colossus_smash;
    }

private:
    static ActionNode* mortal_strike([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("mortal strike",
                /*P*/ nullptr,
                /*A*/ nullptr,
                /*C*/ nullptr);
    }
    static ActionNode* slam([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("slam",
                /*P*/ nullptr,
                /*A*/ nullptr,
                /*C*/ nullptr);
    }

    static ActionNode* overpower([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("overpower",
                /*P*/ nullptr,
                /*A*/ nullptr,
                /*C*/ nullptr);
    }
    static ActionNode* die_by_the_sword([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("die by the sword",
                /*P*/ nullptr,
                /*A*/ nullptr,
                /*C*/ nullptr);
    }
    static ActionNode* sweeping_strikes([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("sweeping strikes",
                /*P*/ nullptr,
                /*A*/ nullptr,
                /*C*/ nullptr);
    }
    static ActionNode* colossus_smash([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("colossus smash",
                /*P*/ nullptr,
                /*A*/ nullptr,
                /*C*/ nullptr);
    }
};

ArmsWarriorStrategy::ArmsWarriorStrategy(PlayerbotAI* botAI) : GenericWarriorStrategy(botAI)
{
    actionNodeFactories.Add(new ArmsWarriorStrategyActionNodeFactory());
}

NextAction** ArmsWarriorStrategy::getDefaultActions()
{
    return NextAction::array(0,
                             new NextAction("mortal strike", ACTION_NORMAL + 10.0f),
                             new NextAction("colossus smash", ACTION_NORMAL + 10.0f),
                             new NextAction("slam", ACTION_NORMAL + 1.0f),
                             new NextAction("heroic strike", ACTION_DEFAULT),
                             nullptr);
}

void ArmsWarriorStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    GenericWarriorStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode("battle shout",
                                       NextAction::array(0, new NextAction("battle shout", ACTION_HIGH + 9), nullptr)));
}
