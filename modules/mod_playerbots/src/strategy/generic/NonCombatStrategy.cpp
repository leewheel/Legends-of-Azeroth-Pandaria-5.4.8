#include "NonCombatStrategy.h"

#include "Playerbots.h"

void NonCombatStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("random", NextAction::array(0, new NextAction("clean quest log", 1.0f), nullptr)));
    triggers.push_back(new TriggerNode("timer", NextAction::array(0, new NextAction("check mount state", 1.0f),
        // new NextAction("check values", 1.0f), 
        nullptr)));
    // triggers.push_back(new TriggerNode("near dark portal", NextAction::array(0, new NextAction("move to dark portal", 1.0f), nullptr)));
    // triggers.push_back(new TriggerNode("at dark portal azeroth", NextAction::array(0, new NextAction("use dark portal azeroth", 1.0f), nullptr)));
    // triggers.push_back(new TriggerNode("at dark portal outland", NextAction::array(0, new NextAction("move from dark portal", 1.0f), nullptr)));
    // triggers.push_back(new TriggerNode("vehicle near", NextAction::array(0, new NextAction("enter vehicle", 10.0f), nullptr)));
}