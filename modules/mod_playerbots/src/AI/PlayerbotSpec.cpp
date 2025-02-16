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

    const Specializations& spec = player->GetSpecialization();
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
            if (spec == Specializations::SPEC_DRUID_FERAL || spec == Specializations::SPEC_DRUID_GUARDIAN)
            {
                return false;
            }
            break;
        }
        case CLASS_PALADIN:
        {
            if (spec != Specializations::SPEC_DEATH_KNIGHT_UNHOLY)
            {
                return false;
            }
            break;
        }
        case CLASS_SHAMAN:
        {
            if (spec == Specializations::SPEC_SHAMAN_ENHANCEMENT)
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

    if (player->GetSpecialization() == Specializations::SPEC_PALADIN_PROTECTION ||
        player->GetSpecialization() == Specializations::SPEC_WARRIOR_PROTECTION ||
        player->GetSpecialization() == Specializations::SPEC_MONK_BREWMASTER ||
        player->GetSpecialization() == Specializations::SPEC_DRUID_GUARDIAN ||
        player->GetSpecialization() == Specializations::SPEC_DEATH_KNIGHT_BLOOD)
        return true;
    return false;
}

bool PlayerBotSpec::IsHeal(Player* player, bool bySpec)
{
    PlayerbotAI* botAi = GET_PLAYERBOT_AI(player);
    if (!bySpec && botAi)
        return botAi->ContainsStrategy(STRATEGY_TYPE_HEAL);

    if (player->GetSpecialization() == Specializations::SPEC_PALADIN_HOLY ||
        player->GetSpecialization() == Specializations::SPEC_PRIEST_DISCIPLINE||
        player->GetSpecialization() == Specializations::SPEC_PRIEST_HOLY ||
        player->GetSpecialization() == Specializations::SPEC_DRUID_RESTORATION ||
        player->GetSpecialization() == Specializations::SPEC_SHAMAN_RESTORATION ||
        player->GetSpecialization() == Specializations::SPEC_MONK_MISTWEAVER)
        return true;

    return false;
}

bool PlayerBotSpec::IsDps(Player* player, bool bySpec)
{
    PlayerbotAI* botAi = GET_PLAYERBOT_AI(player);
    if (!bySpec && botAi)
        return botAi->ContainsStrategy(STRATEGY_TYPE_DPS);

    return (!IsTank(player) && !IsHeal(player));
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