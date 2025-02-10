/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#include "AiFactory.h"

#include "BotAIObjectContext.h"

#include "Engine.h"
#include "Group.h"

#include "Item.h"

#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "PlayerbotSpec.h"

#include "SharedDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

AiObjectContext* AiFactory::createAiObjectContext(Player* player, PlayerbotAI* botAI)
{
    /*switch (player->getClass())
    {
        case CLASS_PRIEST:
            return new PriestAiObjectContext(botAI);
        case CLASS_MAGE:
            return new MageAiObjectContext(botAI);
        case CLASS_WARLOCK:
            return new WarlockAiObjectContext(botAI);
        case CLASS_WARRIOR:
            return new WarriorAiObjectContext(botAI);
        case CLASS_SHAMAN:
            return new ShamanAiObjectContext(botAI);
        case CLASS_PALADIN:
            return new PaladinAiObjectContext(botAI);
        case CLASS_DRUID:
            return new DruidAiObjectContext(botAI);
        case CLASS_HUNTER:
            return new HunterAiObjectContext(botAI);
        case CLASS_ROGUE:
            return new RogueAiObjectContext(botAI);
        case CLASS_DEATH_KNIGHT:
            return new DKAiObjectContext(botAI);
    }*/

    return new BotAiObjectContext(botAI);
}

uint8 AiFactory::GetPlayerSpecTab(Player* bot)
{
    return 0;
}

std::map<uint8, uint32> AiFactory::GetPlayerSpecTabs(Player* bot)
{
    std::map<uint8, uint32> tabs = {{0, 0}, {0, 0}, {0, 0}};
    /*const PlayerTalentMap& talentMap = bot->GetTalentMap();
    for (PlayerTalentMap::const_iterator i = talentMap.begin(); i != talentMap.end(); ++i)
    {
        uint32 spellId = i->first;
        if ((bot->GetActiveSpecMask() & i->second->specMask) == 0)
        {
            continue;
        }
        TalentSpellPos const* talentPos = GetTalentSpellPos(spellId);
        if (!talentPos)
            continue;
        TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentPos->talent_id);
        if (!talentInfo)
            continue;

        uint32 const* talentTabIds = GetTalentTabPages(bot->getClass());

        const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        int rank = spellInfo ? spellInfo->GetRank() : 1;
        if (talentInfo->TalentTab == talentTabIds[0])
            tabs[0] += rank;
        if (talentInfo->TalentTab == talentTabIds[1])
            tabs[1] += rank;
        if (talentInfo->TalentTab == talentTabIds[2])
            tabs[2] += rank;
    }*/
    return tabs;
}

BotRoles AiFactory::GetPlayerRoles(Player* player)
{
    BotRoles role = BOT_ROLE_NONE;
    uint8 tab = GetPlayerSpecTab(player);

    switch (player->GetClass())
    {
        case CLASS_PRIEST:
            if (tab == PRIEST_TABS::PRIEST_TAB_SHADOW)
                role = BOT_ROLE_DPS;
            else
                role = BOT_ROLE_HEALER;
            break;
        case CLASS_SHAMAN:
            if (tab == SHAMAN_TABS::SHAMAN_TAB_RESTORATION)
                role = BOT_ROLE_HEALER;
            else
                role = BOT_ROLE_DPS;
            break;
        case CLASS_WARRIOR:
            if (tab == WARRIOR_TABS::WARRIOR_TAB_PROTECTION)
                role = BOT_ROLE_TANK;
            else
                role = BOT_ROLE_DPS;
            break;
        case CLASS_PALADIN:
            if (tab == PALADIN_TABS::PALADIN_TAB_HOLY)
                role = BOT_ROLE_HEALER;
            else if (tab == PALADIN_TABS::PALADIN_TAB_PROTECTION)
                role = BOT_ROLE_TANK;
            else if (tab == 2)
                role = BOT_ROLE_DPS;
            break;
        case CLASS_DRUID:
            if (tab == DRUID_TABS::DRUID_TAB_FERAL || tab == DRUID_TABS::DRUID_TAB_BALANCE)
                role = BOT_ROLE_DPS;
            else if (tab == DRUID_TABS::DRUID_TAB_GUARDIAN)
                role = BOT_ROLE_TANK;
            else if (tab == DRUID_TABS::DRUID_TAB_RESTORATION)
                role = BOT_ROLE_HEALER;
            break;
        case CLASS_MONK:
            if (tab == MONK_TABS::MONK_TAB_BREWMASTER)
                role = BOT_ROLE_TANK;
            else if (tab == MONK_TABS::MONK_TAB_MISTWEAVER)
                role = BOT_ROLE_HEALER;
            else if (tab == MONK_TABS::MONK_TAB_WINDWALKER)
                role = BOT_ROLE_DPS;
            break;
        default:
            role = BOT_ROLE_DPS;
            break;
    }

    return role;
}

std::string AiFactory::GetPlayerSpecName(Player* player)
{
    std::string specName;
    int tab = GetPlayerSpecTab(player);
    switch (player->GetClass())
    {
        case CLASS_PRIEST:
            if (tab == PRIEST_TABS::PRIEST_TAB_SHADOW)
                specName = "shadow";
            else if (tab == PRIEST_TABS::PRIEST_TAB_HOLY)
                specName = "holy";
            else
                specName = "disc";
            ;
            break;
        case CLASS_SHAMAN:
            if (tab == SHAMAN_TABS::SHAMAN_TAB_RESTORATION)
                specName = "resto";
            else if (tab == SHAMAN_TABS::SHAMAN_TAB_ENHANCEMENT)
                specName = "enhance";
            else
                specName = "elem";
            break;
        case CLASS_WARRIOR:
            if (tab == WARRIOR_TABS::WARRIOR_TAB_PROTECTION)
                specName = "prot";
            else if (tab == WARRIOR_TABS::WARRIOR_TAB_FURY)
                specName = "fury";
            else
                specName = "arms";
            break;
        case CLASS_PALADIN:
            if (tab == PALADIN_TABS::PALADIN_TAB_HOLY)
                specName = "holy";
            else if (tab == PALADIN_TABS::PALADIN_TAB_PROTECTION)
                specName = "prot";
            else if (tab == PALADIN_TABS::PALADIN_TAB_RETRIBUTION)
                specName = "retrib";
            break;
        case CLASS_DRUID:
            if (tab == DRUID_TABS::DRUID_TAB_BALANCE)
                specName = "balance";
            else if (tab == DRUID_TABS::DRUID_TAB_FERAL)
                specName = "feraldps";
            else if (tab == DRUID_TABS::DRUID_TAB_GUARDIAN)
                specName = "guardiantank";
            else if (tab == DRUID_TABS::DRUID_TAB_RESTORATION)
                specName = "resto";
            break;
        case CLASS_ROGUE:
            if (tab == ROGUE_TABS::ROGUE_TAB_ASSASSINATION)
                specName = "assas";
            else if (tab == ROGUE_TABS::ROGUE_TAB_COMBAT)
                specName = "combat";
            else if (tab == ROGUE_TABS::ROGUE_TAB_SUBTLETY)
                specName = "subtle";
            break;
        case CLASS_HUNTER:
            if (tab == HUNTER_TABS::HUNTER_TAB_BEASTMASTER)
                specName = "beast";
            else if (tab == HUNTER_TABS::HUNTER_TAB_MARKSMANSHIP)
                specName = "marks";
            else if (tab == HUNTER_TABS::HUNTER_TAB_SURVIVAL)
                specName = "surv";
            break;
        case CLASS_DEATH_KNIGHT:
            if (tab == DEATHKNIGHT_TABS::DEATHKNIGHT_TAB_BLOOD)
                specName = "blooddps";
            else if (tab == DEATHKNIGHT_TABS::DEATHKNIGHT_TAB_FROST)
                specName = "frostdps";
            else if (tab == DEATHKNIGHT_TABS::DEATHKNIGHT_TAB_UNHOLY)
                specName = "unholydps";
            break;
        case CLASS_MAGE:
            if (tab == MAGE_TABS::MAGE_TAB_ARCANE)
                specName = "arcane";
            else if (tab == MAGE_TABS::MAGE_TAB_FIRE)
                specName = "fire";
            else if (tab == MAGE_TABS::MAGE_TAB_FROST)
                specName = "frost";
            break;
        case CLASS_WARLOCK:
            if (tab == WARLOCK_TABS::WARLOCK_TAB_AFFLICATION)
                specName = "afflic";
            else if (tab == WARLOCK_TABS::WARLOCK_TAB_DEMONOLOGY)
                specName = "demo";
            else if (tab == WARLOCK_TABS::WARLOCK_TAB_DESTRUCTION)
                specName = "destro";
            break;
        case CLASS_MONK:
            if (tab == MONK_TABS::MONK_TAB_BREWMASTER)
                specName = "brewmaster";
            else if (tab == MONK_TABS::MONK_TAB_MISTWEAVER)
                specName = "mistweaver";
            else if (tab == MONK_TABS::MONK_TAB_WINDWALKER)
                specName = "windwalker";
        default:
            break;
    }

    return specName;
}

void AiFactory::AddDefaultCombatStrategies(Player* player, PlayerbotAI* const facade, Engine* engine)
{
    if (!player->InBattleground())
    {
        engine->addStrategiesNoInit("racials", "chat", "default", "cast time", "duel", "boost", nullptr);
    }
    if (sPlayerbotAIConfig->autoAvoidAoe && facade->HasRealPlayerMaster())
    {
        engine->addStrategy("avoid aoe", false);
    }
    engine->addStrategy("formation", false);

    const uint8& tab = GetPlayerSpecTab(player);
    switch (player->GetClass())
    {
        case CLASS_PRIEST:
        {
            if (tab == PRIEST_TABS::PRIEST_TAB_SHADOW)
            {
                engine->addStrategiesNoInit("dps", "shadow debuff", "shadow aoe", nullptr);
            }
            else if (tab == PRIEST_TABS::PRIEST_TAB_DISIPLINE)
            {
                engine->addStrategiesNoInit("heal", nullptr);
            }
            else
            {
                engine->addStrategiesNoInit("holy heal", nullptr);
            }

            engine->addStrategiesNoInit("dps assist", "cure", nullptr);
            break;
        }
        case CLASS_MAGE:
        {
            if (tab == MAGE_TABS::MAGE_TAB_ARCANE)
                engine->addStrategiesNoInit("arcane", "arcane aoe", nullptr);
            else if (tab == MAGE_TABS::MAGE_TAB_FIRE)
                engine->addStrategiesNoInit("fire", "fire aoe", nullptr);
            else
                engine->addStrategiesNoInit("frost", "frost aoe", nullptr);

            engine->addStrategiesNoInit("dps", "dps assist", "cure", nullptr);
            break;
        }
        case CLASS_WARRIOR:
        {
            if (tab == WARRIOR_TABS::WARRIOR_TAB_PROTECTION)
                engine->addStrategiesNoInit("tank", "tank assist", "aoe", nullptr);
            else if (player->GetLevel() < 36 || tab == WARRIOR_TABS::WARRIOR_TAB_ARMS)
                engine->addStrategiesNoInit("arms", "aoe", "dps assist", /*"behind",*/ nullptr);
            else
                engine->addStrategiesNoInit("fury", "aoe", "dps assist", /*"behind",*/ nullptr);
            break;
        }
        case CLASS_SHAMAN:
        {
            if (tab == SHAMAN_TABS::SHAMAN_TAB_ELEMENTAL)
                engine->addStrategiesNoInit("caster", "caster aoe", "bmana", nullptr);
            else if (tab == SHAMAN_TABS::SHAMAN_TAB_RESTORATION)
                engine->addStrategiesNoInit("heal", "bmana", nullptr);
            else
                engine->addStrategiesNoInit("melee", "melee aoe", "bdps", nullptr);

            engine->addStrategiesNoInit("dps assist", "cure", "totems", nullptr);
            break;
        }
        case CLASS_PALADIN:
        {
            if (tab == PALADIN_TABS::PALADIN_TAB_PROTECTION)
                engine->addStrategiesNoInit("tank", "tank assist", "bthreat", "barmor", "cure", nullptr);
            else if (tab == PALADIN_TABS::PALADIN_TAB_HOLY)
                engine->addStrategiesNoInit("heal", "dps assist", "cure", "bcast", nullptr);
            else
                engine->addStrategiesNoInit("dps", "dps assist", "cure", "baoe", nullptr);
            break;
        }
        case CLASS_DRUID:
        {
            if (tab == DRUID_TABS::DRUID_TAB_BALANCE)
            {
                engine->addStrategiesNoInit("caster", "cure", "caster aoe", "dps assist", nullptr);
                engine->addStrategy("caster debuff", false);
            }
            else if (tab == DRUID_TABS::DRUID_TAB_RESTORATION)
                engine->addStrategiesNoInit("heal", "cure", "dps assist", nullptr);
            else if (tab == DRUID_TABS::DRUID_TAB_FERAL)
            {
                if (player->GetLevel() >= 20 && !player->HasAura(16931) /*thick hide*/)
                {
                    engine->addStrategiesNoInit("cat", "dps assist", nullptr);
                }
                else
                {
                    engine->addStrategiesNoInit("bear", "tank assist", nullptr);
                }
            }
            else if (tab == DRUID_TABS::DRUID_TAB_GUARDIAN)
            {
                engine->addStrategiesNoInit("bear", "tank assist", nullptr);
            }
            break;
        }
        case CLASS_HUNTER:
        {
            engine->addStrategiesNoInit("dps", "aoe", "bdps", "dps assist", nullptr);
            engine->addStrategy("dps debuff", false);
            break;
        }
        case CLASS_ROGUE:
        {
            if (tab == ROGUE_TAB_ASSASSINATION)
            {
                engine->addStrategiesNoInit("melee", "dps assist", "aoe", /*"behind",*/ nullptr);
            }
            else
            {
                engine->addStrategiesNoInit("dps", "dps assist", "aoe", /*"behind",*/ nullptr);
            }
            break;
        }
        case CLASS_WARLOCK:
        {
            engine->addStrategiesNoInit("dps assist", "dps", "dps debuff", "aoe", nullptr);
            break;
        }
        case CLASS_DEATH_KNIGHT:
        {
            if (tab == DEATHKNIGHT_TABS::DEATHKNIGHT_TAB_BLOOD)
                engine->addStrategiesNoInit("blood", "tank assist", nullptr);
            else if (tab == DEATHKNIGHT_TABS::DEATHKNIGHT_TAB_FROST)
                engine->addStrategiesNoInit("frost", "frost aoe", "dps assist", nullptr);
            else
                engine->addStrategiesNoInit("unholy", "unholy aoe", "dps assist", nullptr);
            break;
        }
        case CLASS_MONK:
        {
            if (tab == MONK_TABS::MONK_TAB_BREWMASTER)
                engine->addStrategiesNoInit("tank", "tank assist", "aoe", nullptr);
            else if (tab == MONK_TABS::MONK_TAB_MISTWEAVER)
                engine->addStrategiesNoInit("heal", nullptr);
            else
                engine->addStrategiesNoInit("melee", "melee aoe", "dps assist", nullptr);

            engine->addStrategiesNoInit("dps assist", "cure", nullptr);
        }
        break;
    }

    // temporary
    engine->addStrategy("say hello");
}

Engine* AiFactory::createCombatEngine(Player* player, PlayerbotAI* const facade, AiObjectContext* aiObjectContext)
{
    Engine* engine = new Engine(facade, aiObjectContext);
    AddDefaultCombatStrategies(player, facade, engine);
    engine->Init();
    return engine;
}

void AiFactory::AddDefaultNonCombatStrategies(Player* player, PlayerbotAI* const facade, Engine* nonCombatEngine)
{
    if (!player->InBattleground())
    {
        nonCombatEngine->addStrategiesNoInit("nc", "food", "chat", "follow", "default", "quest", "loot", "gather", "duel",
            "buff", "mount", "emote", nullptr);
    }
    nonCombatEngine->addStrategy("say hello");
}

Engine* AiFactory::createNonCombatEngine(Player* player, PlayerbotAI* const facade, AiObjectContext* aiObjectContext)
{
    Engine* nonCombatEngine = new Engine(facade, aiObjectContext);

    AddDefaultNonCombatStrategies(player, facade, nonCombatEngine);
    nonCombatEngine->Init();
    return nonCombatEngine;
}

void AiFactory::AddDefaultDeadStrategies(Player* player, PlayerbotAI* const facade, Engine* deadEngine)
{
    (void)facade;  // unused and remove warning
    deadEngine->addStrategiesNoInit("dead", "stay", "chat", "default", "follow", nullptr);

    if (sRandomPlayerbotMgr->IsRandomBot(player) && !player->GetGroup())
    {
        deadEngine->removeStrategy("follow", false);
    }
}

Engine* AiFactory::createDeadEngine(Player* player, PlayerbotAI* const facade, AiObjectContext* AiObjectContext)
{
    Engine* deadEngine = new Engine(facade, AiObjectContext);
    AddDefaultDeadStrategies(player, facade, deadEngine);
    deadEngine->Init();
    return deadEngine;
}
