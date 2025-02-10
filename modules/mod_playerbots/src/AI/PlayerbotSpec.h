#ifndef _PLAYERBOT_PLAYERBOTSPEC_H
#define _PLAYERBOT_PLAYERBOTSPEC_H

enum HUNTER_TABS
{
    HUNTER_TAB_BEASTMASTER,
    HUNTER_TAB_MARKSMANSHIP,
    HUNTER_TAB_SURVIVAL,
};

enum ROGUE_TABS
{
    ROGUE_TAB_ASSASSINATION,
    ROGUE_TAB_COMBAT,
    ROGUE_TAB_SUBTLETY
};

enum PRIEST_TABS
{
    PRIEST_TAB_DISIPLINE,
    PRIEST_TAB_HOLY,
    PRIEST_TAB_SHADOW,
};

enum DEATHKNIGHT_TABS
{
    DEATHKNIGHT_TAB_BLOOD,
    DEATHKNIGHT_TAB_FROST,
    DEATHKNIGHT_TAB_UNHOLY,
};

enum DRUID_TABS
{
    DRUID_TAB_BALANCE,
    DRUID_TAB_FERAL,
    DRUID_TAB_GUARDIAN,
    DRUID_TAB_RESTORATION,
};

enum MAGE_TABS
{
    MAGE_TAB_ARCANE,
    MAGE_TAB_FIRE,
    MAGE_TAB_FROST,
};

enum SHAMAN_TABS
{
    SHAMAN_TAB_ELEMENTAL,
    SHAMAN_TAB_ENHANCEMENT,
    SHAMAN_TAB_RESTORATION,
};

enum PALADIN_TABS
{
    PALADIN_TAB_HOLY,
    PALADIN_TAB_PROTECTION,
    PALADIN_TAB_RETRIBUTION,
};

enum WARLOCK_TABS
{
    WARLOCK_TAB_AFFLICATION,
    WARLOCK_TAB_DEMONOLOGY,
    WARLOCK_TAB_DESTRUCTION,
};

enum WARRIOR_TABS
{
    WARRIOR_TAB_ARMS,
    WARRIOR_TAB_FURY,
    WARRIOR_TAB_PROTECTION,
};

enum MONK_TABS
{
    MONK_TAB_BREWMASTER,
    MONK_TAB_MISTWEAVER,
    MONK_TAB_WINDWALKER
};

class Player;
class PlayerBotSpec
{
public:
    // 
    static bool IsTank(Player* player, bool bySpec = false);
    static bool IsHeal(Player* player, bool bySpec = false);
    static bool IsDps(Player* player, bool bySpec = false);
    static bool IsRanged(Player* player, bool bySpec = false);
    static bool IsMelee(Player* player, bool bySpec = false);
    static bool IsCaster(Player* player, bool bySpec = false);
    static bool IsCombo(Player* player, bool bySpec = false);
    static bool IsRangedDps(Player* player, bool bySpec = false);
    static bool IsMainTank(Player* player);
};

#endif