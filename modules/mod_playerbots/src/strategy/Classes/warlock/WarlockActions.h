/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_WARLOCKACTIONS_H
#define _PLAYERBOT_WARLOCKACTIONS_H

#include "GenericSpellActions.h"
#include "Playerbots.h"

class PlayerbotAI;
class Unit;


SPELL_ACTION(CastSoulShatterAction, "soulshatter");
SPELL_ACTION(CastUnendingResolveAction, "unending resolve");
SPELL_ACTION(CastFelFlameAction, "fel flame");
BUFF_ACTION(CastDarkIntentAction, "dark intent");
SPELL_ACTION(CastDarkSoulAction, "dark soul");
SPELL_ACTION(CastTwilightWardAction, "twilight ward");

BEGIN_RANGED_SPELL_ACTION(CastShadowBoltAction, "shadow bolt")
END_SPELL_ACTION()

class CastDrainSoulAction : public CastSpellAction
{
public:
    CastDrainSoulAction(PlayerbotAI* botAI) : CastSpellAction(botAI, "drain soul") {}

    bool isUseful() override;
};

class CastDrainLifeAction : public CastSpellAction
{
public:
    CastDrainLifeAction(PlayerbotAI* botAI) : CastSpellAction(botAI, "drain life") {}
};
class CastCurseOfWeaknessAction : public CastDebuffSpellAction
{
public:
    CastCurseOfWeaknessAction(PlayerbotAI* botAI) : CastDebuffSpellAction(botAI, "curse of weakness") {}
};

class CastCorruptionAction : public CastDebuffSpellAction
{
public:
    CastCorruptionAction(PlayerbotAI* botAI) : CastDebuffSpellAction(botAI, "corruption", true) {}
    bool isUseful() override
    {
        return CastDebuffSpellAction::isUseful() && !botAI->HasAura("seed of corruption", GetTarget(), false, true);
    }
};
class CastCorruptionOnAttackerAction : public CastDebuffSpellOnAttackerAction
{
public:
    CastCorruptionOnAttackerAction(PlayerbotAI* botAI) : CastDebuffSpellOnAttackerAction(botAI, "corruption", true) {}
    bool isUseful() override
    {
        return CastDebuffSpellOnAttackerAction::isUseful() &&
               !botAI->HasAura("seed of corruption", GetTarget(), false, true);
    }
};
class CastSummonVoidwalkerAction : public CastBuffSpellAction
{
public:
    CastSummonVoidwalkerAction(PlayerbotAI* botAI) : CastBuffSpellAction(botAI, "summon voidwalker") {}
};

class CastSummonFelguardAction : public CastBuffSpellAction
{
public:
    CastSummonFelguardAction(PlayerbotAI* botAI) : CastBuffSpellAction(botAI, "summon felguard") {}
};

class CastSummonFelhunterAction : public CastBuffSpellAction
{
public:
    CastSummonFelhunterAction(PlayerbotAI* botAI) : CastBuffSpellAction(botAI, "summon felhunter") {}
};

class CastSummonImpAction : public CastBuffSpellAction
{
public:
    CastSummonImpAction(PlayerbotAI* botAI) : CastBuffSpellAction(botAI, "summon imp") {}
};

class CastSummonSuccubusAction : public CastBuffSpellAction
{
public:
    CastSummonSuccubusAction(PlayerbotAI* botAI) : CastBuffSpellAction(botAI, "summon succubus") {}
};

class CastCreateHealthstoneAction : public CastBuffSpellAction
{
public:
    CastCreateHealthstoneAction(PlayerbotAI* botAI) : CastBuffSpellAction(botAI, "create healthstone") {}
};

class CastBanishAction : public CastBuffSpellAction
{
public:
    CastBanishAction(PlayerbotAI* botAI) : CastBuffSpellAction(botAI, "banish on cc") {}

    Value<Unit*>* GetTargetValue() override;
    bool Execute(Event event) override;
};
class CastRainOfFireAction : public CastSpellAction
{
public:
    CastRainOfFireAction(PlayerbotAI* botAI) : CastSpellAction(botAI, "rain of fire") {}
    ActionThreatType getThreatType() override { return ActionThreatType::Aoe; }
};

class CastShadowfuryAction : public CastSpellAction
{
public:
    CastShadowfuryAction(PlayerbotAI* botAI) : CastSpellAction(botAI, "shadowfury") {}
};

class CastImmolateAction : public CastDebuffSpellAction
{
public:
    CastImmolateAction(PlayerbotAI* botAI) : CastDebuffSpellAction(botAI, "immolate", true) {}
};

class CastImmolateOnAttackerAction : public CastDebuffSpellOnAttackerAction
{
public:
    CastImmolateOnAttackerAction(PlayerbotAI* botAI) : CastDebuffSpellOnAttackerAction(botAI, "immolate", true) {}
};

class CastConflagrateAction : public CastSpellAction
{
public:
    CastConflagrateAction(PlayerbotAI* botAI) : CastSpellAction(botAI, "conflagrate") {}
};

class CastIncinirateAction : public CastSpellAction
{
public:
    CastIncinirateAction(PlayerbotAI* botAI) : CastSpellAction(botAI, "incinirate") {}
};

class CastFearAction : public CastDebuffSpellAction
{
public:
    CastFearAction(PlayerbotAI* botAI) : CastDebuffSpellAction(botAI, "fear") {}
};

class CastFearOnCcAction : public CastBuffSpellAction
{
public:
    CastFearOnCcAction(PlayerbotAI* botAI) : CastBuffSpellAction(botAI, "fear on cc") {}

    Value<Unit*>* GetTargetValue() override;
    bool Execute(Event event) override;
    bool isPossible() override;
    bool isUseful() override;
};

class CastLifeTapAction : public CastSpellAction
{
public:
    CastLifeTapAction(PlayerbotAI* botAI) : CastSpellAction(botAI, "life tap") {}

    std::string const GetTargetName() override { return "self target"; }
    bool isUseful() override;
};

class CastAmplifyCurseAction : public CastBuffSpellAction
{
public:
    CastAmplifyCurseAction(PlayerbotAI* botAI) : CastBuffSpellAction(botAI, "amplify curse") {}
};

class CastSiphonLifeAction : public CastDebuffSpellAction
{
public:
    CastSiphonLifeAction(PlayerbotAI* botAI) : CastDebuffSpellAction(botAI, "siphon life", true) {}
};

class CastSiphonLifeOnAttackerAction : public CastDebuffSpellOnAttackerAction
{
public:
    CastSiphonLifeOnAttackerAction(PlayerbotAI* botAI) : CastDebuffSpellOnAttackerAction(botAI, "siphon life") {}
};
class CastDemonicEmpowermentAction : public CastBuffSpellAction
{
public:
    CastDemonicEmpowermentAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "demonic empowerment") {}
    std::string const GetTargetName() override { return "pet target"; }
};

class CastMetamorphosisAction : public CastBuffSpellAction
{
public:
    CastMetamorphosisAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "metamorphosis") {}
};
class CastSoulFireAction : public CastSpellAction
{
public:
    CastSoulFireAction(PlayerbotAI* ai) : CastSpellAction(ai, "soul fire") {}
};

class CastIncinerateAction : public CastSpellAction
{
public:
    CastIncinerateAction(PlayerbotAI* ai) : CastSpellAction(ai, "incinerate") {}
};


// -- Affliction
SPELL_ACTION(CastMaleficGraspAction, "malefic grasp");
class CastCurseOfAgonyAction : public CastDebuffSpellAction
{
public:
    CastCurseOfAgonyAction(PlayerbotAI* botAI) : CastDebuffSpellAction(botAI, "agony", true) {}
};
class CastUnstableAfflictionAction : public CastDebuffSpellAction
{
public:
    CastUnstableAfflictionAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "unstable affliction", true) {}
};
class CastUnstableAfflictionOnAttackerAction : public CastDebuffSpellOnAttackerAction
{
public:
    CastUnstableAfflictionOnAttackerAction(PlayerbotAI* ai)
        : CastDebuffSpellOnAttackerAction(ai, "unstable affliction", true)
    {
    }
};
class CastHauntAction : public CastSpellAction
{
public:
    CastHauntAction(PlayerbotAI* ai) : CastSpellAction(ai, "haunt") {}
};
class CastSeedOfCorruptionAction : public CastDebuffSpellAction
{
public:
    CastSeedOfCorruptionAction(PlayerbotAI* botAI) : CastDebuffSpellAction(botAI, "seed of corruption", true, 0) {}
    bool isUseful() override
    {
        return CastDebuffSpellAction::isUseful() && !botAI->HasAura("corruption", GetTarget(), false, true);
    }
    ActionThreatType getThreatType() override { return ActionThreatType::Aoe; }
};

class CastSeedOfCorruptionOnAttackerAction : public CastDebuffSpellOnAttackerAction
{
public:
    CastSeedOfCorruptionOnAttackerAction(PlayerbotAI* botAI)
        : CastDebuffSpellOnAttackerAction(botAI, "seed of corruption", true, 0)
    {
    }
    bool isUseful() override
    {
        return CastDebuffSpellOnAttackerAction::isUseful() && !botAI->HasAura("corruption", GetTarget(), false, true);
    }
    ActionThreatType getThreatType() override { return ActionThreatType::Aoe; }
};
class CastCurseOfAgonyOnAttackerAction : public CastDebuffSpellOnAttackerAction
{
public:
    CastCurseOfAgonyOnAttackerAction(PlayerbotAI* botAI)
        : CastDebuffSpellOnAttackerAction(botAI, "agony", true)
    {
    }
};
#endif
