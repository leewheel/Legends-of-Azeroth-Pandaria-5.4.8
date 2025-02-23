/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#include "GenericWarlockStrategy.h"

#include "Playerbots.h"

class GenericWarlockStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    GenericWarlockStrategyActionNodeFactory()
    {
        creators["corruption"] = &corruption;
        creators["curse of enfeeblement"] = curse_of_enfeeblement;
        creators["shadow bolt"] = &shadow_bolt;
        creators["banish"] = &banish;
        creators["fear"] = &fear;
        creators["howl of terror"] = &howl_of_terror;
        creators["drain life"] = &drain_life;
        creators["life tap"] = &life_tap;

        // low threat
        creators["soulshatter"] = &soulshatter;
        // low life cd
        creators["unending resolve"] = &unending_resolve;
        creators["fel flame"] = &fel_flame;
        creators["dark intent"] = &dark_intent;
        creators["dark soul"] = &dark_soul;
        // low life cd
        creators["twilight ward"] = &twilight_ward;

        
    }

private:
    ACTION_NODE_A(corruption, "corruption", "curse of enfeeblement");
    ACTION_NODE(curse_of_enfeeblement, "curse of enfeeblement");
    ACTION_NODE(shadow_bolt, "shadow bolt");
    ACTION_NODE_A(banish, "banish", "fear");
    ACTION_NODE_A(fear, "fear", "howl of terror");
    ACTION_NODE(howl_of_terror, "howl of terror");
    ACTION_NODE(drain_life, "drain life");
    ACTION_NODE(life_tap, "life tap");

    ACTION_NODE(unending_resolve, "unending resolve");
    ACTION_NODE(fel_flame, "fel flame");
    ACTION_NODE(dark_intent, "dark intent");
    ACTION_NODE(dark_soul, "dark soul");
    ACTION_NODE(twilight_ward, "twilight ward");
    ACTION_NODE(soulshatter, "soulshatter");
};

GenericWarlockStrategy::GenericWarlockStrategy(PlayerbotAI* botAI) : RangedCombatStrategy(botAI)
{
    actionNodeFactories.Add(new GenericWarlockStrategyActionNodeFactory());
}

NextAction** GenericWarlockStrategy::getDefaultActions() { return NextAction::array(0, nullptr); }

void GenericWarlockStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    RangedCombatStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode("low mana", NextAction::array(0, new NextAction("life tap", ACTION_EMERGENCY + 5), nullptr)));

    triggers.push_back(new TriggerNode("medium health", NextAction::array(0, new NextAction("twilight ward", ACTION_INTERRUPT), nullptr)));
    triggers.push_back(new TriggerNode("low health", NextAction::array(0, new NextAction("unending resolve", ACTION_INTERRUPT), nullptr)));
    triggers.push_back(new TriggerNode("critical health", NextAction::array(0, new NextAction("drain life", ACTION_INTERRUPT), nullptr)));
}

void WarlockBoostStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("amplify curse", NextAction::array(0, new NextAction("amplify curse", 41.0f), nullptr)));
}

void WarlockCcStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("banish", NextAction::array(0, new NextAction("banish on cc", 32.0f), nullptr)));
    triggers.push_back(new TriggerNode("fear", NextAction::array(0, new NextAction("fear on cc", 33.0f), nullptr)));
}

void AoeWarlockStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(
        new TriggerNode("medium aoe", NextAction::array(0, new NextAction("seed of corruption", 33.0f),
            new NextAction("seed of corruption on attacker", 32.0f),
            new NextAction("rain of fire", 31.0f), nullptr)));
    triggers.push_back(new TriggerNode("corruption on attacker",
        NextAction::array(0, new NextAction("corruption on attacker", 27.0f), nullptr)));
    triggers.push_back(
        new TriggerNode("unstable affliction on attacker",
            NextAction::array(0, new NextAction("unstable affliction on attacker", 26.0f), NULL)));
    triggers.push_back(
        new TriggerNode("curse of agony on attacker",
            NextAction::array(0, new NextAction("curse of agony on attacker", 25.0f), nullptr)));
}
