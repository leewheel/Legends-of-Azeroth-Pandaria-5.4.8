#include "PlayerbotSpec.h"

#include "AiFactory.h"
#include "Playerbots.h"
#include "PlayerbotAI.h"
#include "Player.h"

bool PlayerBotSpec::IsRanged(Player* player, bool bySpec)
{
    PlayerbotAI* botAi = GET_PLAYERBOT_AI(player);
    if (!bySpec && botAi)
        return botAi->ContainsStrategy(STRATEGY_TYPE_RANGED);

    int tab = AiFactory::GetPlayerSpecTab(player);
    switch (player->GetClass())
    {
        case CLASS_DEATH_KNIGHT:
        case CLASS_WARRIOR:
        case CLASS_ROGUE:
        case CLASS_MONK:
        {
            return false;
        }
        case CLASS_DRUID:
        {
            if (tab == DRUID_TABS::DRUID_TAB_FERAL || tab == DRUID_TABS::DRUID_TAB_GUARDIAN)
            {
                return false;
            }
            break;
        }
        case CLASS_PALADIN:
        {
            if (tab != PALADIN_TABS::PALADIN_TAB_HOLY)
            {
                return false;
            }
            break;
        }
        case CLASS_SHAMAN:
        {
            if (tab == SHAMAN_TABS::SHAMAN_TAB_ENHANCEMENT)
            {
                return false;
            }
            break;
        }
    }
    return true;
}

bool PlayerBotSpec::IsMelee(Player* player, bool bySpec)
{
    return !IsRanged(player, bySpec);
}

bool PlayerBotSpec::IsCaster(Player* player, bool bySpec)
{
    return IsRanged(player, bySpec) && player->GetClass() != CLASS_HUNTER;
}

bool PlayerBotSpec::IsCombo(Player* player, bool bySpec)
{
    // int tab = AiFactory::GetPlayerSpecTab(player);
    return player->GetClass() == CLASS_ROGUE ||
        (player->GetClass() == CLASS_DRUID && player->HasAura(768));  // cat druid
}

bool PlayerBotSpec::IsRangedDps(Player* player, bool bySpec)
{
    return IsRanged(player, bySpec) && IsDps(player, bySpec);
}

bool PlayerBotSpec::IsTank(Player* player, bool bySpec)
{
    PlayerbotAI* botAi = GET_PLAYERBOT_AI(player);
    if (!bySpec && botAi)
        return botAi->ContainsStrategy(STRATEGY_TYPE_TANK);

    int tab = AiFactory::GetPlayerSpecTab(player);
    switch (player->GetClass())
    {
        case CLASS_DEATH_KNIGHT:
        {
            if (tab == DEATHKNIGHT_TAB_BLOOD)
            {
                return true;
            }
            break;
        }
        case CLASS_PALADIN:
        {
            if (tab == PALADIN_TAB_PROTECTION)
            {
                return true;
            }
            break;
        }
        case CLASS_WARRIOR:
        {
            if (tab == WARRIOR_TAB_PROTECTION)
            {
                return true;
            }
            break;
        }
        case CLASS_DRUID:
        {
            if (tab == DRUID_TAB_FERAL && (player->GetShapeshiftForm() == FORM_BEAR ||
                player->GetShapeshiftForm() == FORM_DIREBEAR || player->HasAura(16931)))
            {
                return true;
            }
            break;
        }
        case CLASS_MONK:
        {
            if (tab == MONK_TABS::MONK_TAB_BREWMASTER)
                return true;
            break;
        }
    }
    return false;
}

bool PlayerBotSpec::IsHeal(Player* player, bool bySpec)
{
    PlayerbotAI* botAi = GET_PLAYERBOT_AI(player);
    if (!bySpec && botAi)
        return botAi->ContainsStrategy(STRATEGY_TYPE_HEAL);

    int tab = AiFactory::GetPlayerSpecTab(player);
    switch (player->GetClass())
    {
        case CLASS_PRIEST:
        {
            if (tab == PRIEST_TAB_DISIPLINE || tab == PRIEST_TAB_HOLY)
            {
                return true;
            }
            break;
        }
        case CLASS_DRUID:
        {
            if (tab == DRUID_TAB_RESTORATION)
            {
                return true;
            }
            break;
        }
        case CLASS_SHAMAN:
        {
            if (tab == SHAMAN_TAB_RESTORATION)
            {
                return true;
            }
            break;
        }
        case CLASS_PALADIN:
        {
            if (tab == PALADIN_TAB_HOLY)
            {
                return true;
            }
            break;
        }
        case CLASS_MONK:
        {
            if (tab == MONK_TABS::MONK_TAB_MISTWEAVER)
                return true;
            break;
        }
    }
    return false;
}

bool PlayerBotSpec::IsDps(Player* player, bool bySpec)
{
    PlayerbotAI* botAi = GET_PLAYERBOT_AI(player);
    if (!bySpec && botAi)
        return botAi->ContainsStrategy(STRATEGY_TYPE_DPS);

    int tab = AiFactory::GetPlayerSpecTab(player);
    switch (player->GetClass())
    {
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_HUNTER:
        case CLASS_ROGUE:
            return true;
        case CLASS_PRIEST:
        {
            if (tab == PRIEST_TAB_SHADOW)
            {
                return true;
            }
            break;
        }
        case CLASS_DRUID:
        {
            if (tab == DRUID_TAB_BALANCE)
            {
                return true;
            }
            if (tab == DRUID_TAB_FERAL && !IsTank(player, bySpec))
            {
                return true;
            }
            break;
        }
        case CLASS_SHAMAN:
        {
            if (tab != SHAMAN_TAB_RESTORATION)
            {
                return true;
            }
            break;
        }
        case CLASS_PALADIN:
        {
            if (tab == PALADIN_TAB_RETRIBUTION)
            {
                return true;
            }
            break;
        }
        case CLASS_DEATH_KNIGHT:
        {
            if (tab != DEATHKNIGHT_TAB_BLOOD)
            {
                return true;
            }
            break;
        }
        case CLASS_WARRIOR:
        {
            if (tab != WARRIOR_TAB_PROTECTION)
            {
                return true;
            }
            break;
        }
        case CLASS_MONK:
        {
            if (tab == MONK_TABS::MONK_TAB_WINDWALKER)
                return true;
            break;
        }
    }
    return false;
}

bool PlayerBotSpec::IsMainTank(Player* player)
{
    Group* group = player->GetGroup();
    if (!group)
    {
        return false;
    }
    ObjectGuid mainTank = ObjectGuid();
    Group::MemberSlotList const& slots = group->GetMemberSlots();
    for (Group::member_citerator itr = slots.begin(); itr != slots.end(); ++itr)
    {
        if (itr->flags & MEMBER_FLAG_MAINTANK)
        {
            mainTank = itr->guid;
            break;
        }
    }
    if (mainTank != ObjectGuid::Empty)
    {
        return player->GetGUID() == mainTank;
    }
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (IsTank(member) && member->IsAlive())
        {
            return player->GetGUID() == member->GetGUID();
        }
    }
    return false;
}