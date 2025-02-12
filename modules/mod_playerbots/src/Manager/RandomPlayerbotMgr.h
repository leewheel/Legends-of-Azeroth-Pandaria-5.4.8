/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_RANDOMPLAYERBOTMGR_H
#define _PLAYERBOT_RANDOMPLAYERBOTMGR_H

#include "PlayerbotMgr.h"

class CachedEvent
{
public:
    CachedEvent() : value(0), lastChangeTime(0), validIn(0), data("") {}
    CachedEvent(const CachedEvent& other)
        : value(other.value), lastChangeTime(other.lastChangeTime), validIn(other.validIn), data(other.data)
    {
    }
    CachedEvent(uint32 value, uint32 lastChangeTime, uint32 validIn, std::string const data = "")
        : value(value), lastChangeTime(lastChangeTime), validIn(validIn), data(data)
    {
    }

    bool IsEmpty() { return !lastChangeTime; }

public:
    uint32 value;
    uint32 lastChangeTime;
    uint32 validIn;
    std::string data;
};

class Player;
class ObjectGuid;

class RandomPlayerbotMgr : public PlayerbotHolder
{
public:
    RandomPlayerbotMgr();
    virtual ~RandomPlayerbotMgr();
    static RandomPlayerbotMgr* instance()
    {
        static RandomPlayerbotMgr instance;
        return &instance;
    }

    /// <summary>
    /// Allow pre load reserve on internal containers
    /// </summary>
    /// <param name="size"></param>
    void Reserve(const uint32 size);
    void UpdateAIInternal(uint32 elapsed, bool minimal = false) override;

private:
    struct farm_zone {
        uint32 zone_id;
        uint32 zone_type;
        uint32 min_level;
        uint32 max_level;
        Team team_disabled;
        uint32 map_id;
        uint32 min_player;
        uint32 max_player;
    };

    struct farm_spot {
        uint32 zone_id;
        uint32 id;
        uint32 min_level;
        uint32 max_level;
        Team team_disabled;
        float x;
        float y;
        float z;
        uint32 radius;
    };

public:
    uint32 activeBots = 0;
    bool IsRandomBot(Player* bot);
    bool IsRandomBot(ObjectGuid::LowType bot);
    void Clear(Player* bot);
    void OnPlayerLogout(Player* player);
    void OnPlayerLogin(Player* player);
    void OnPlayerLoginError(uint32 bot);
    Player* GetRandomPlayer();
    std::vector<Player*> GetPlayers() { return _players; };
    PlayerBotMap GetAllBots() { return playerBots; };
    void Refresh(Player* bot);
    uint32 GetMaxAllowedBotCount();
    bool ProcessBot(Player* player);
    void Revive(Player* player);
    uint32 GetValue(Player* bot, std::string const type);
    uint32 GetValue(uint32 bot, std::string const type);
    std::string const GetData(uint32 bot, std::string const type);
    void SetValue(uint32 bot, std::string const type, uint32 value, std::string const data = "");
    void SetValue(Player* bot, std::string const type, uint32 value, std::string const data = "");
    void Remove(Player* bot);
    void CheckPlayers();
    float getActivityMod() { return _activityMod; }
    float getActivityPercentage() { return _activityMod * 100.0f; }
    void setActivityPercentage(float percentage) { _activityMod = percentage / 100.0f; }
    static uint8 GetTeamClassIdx(bool isAlliance, uint8 claz) { return isAlliance * 20 + claz; }
    void PrepareAddclassCache();
    void PrepareTeleportCache();
    std::map<uint8, std::vector<ObjectGuid>>& AddclassCache() { return _addclassCache; };
protected:
    void OnBotLoginInternal(Player* const bot) override;

private:
    void GetBots();
    uint32 AddRandomBots();
    bool ProcessBot(uint32 bot);

    // pid values are set in constructor
    float _activityMod = 0.25;
    bool _isBotInitializing = true;
    uint32 GetEventValue(uint32 bot, std::string const event);
    std::string const GetEventData(uint32 bot, std::string const event);
    uint32 SetEventValue(uint32 bot, std::string const event, uint32 value, uint32 validIn, std::string const data = "");
    time_t _playersCheckTimer;
    typedef void (RandomPlayerbotMgr::* ConsoleCommandHandler)(Player*);
    std::vector<Player*> _players;
    uint32 _processTicks;
    std::map<uint32, std::map<std::string, CachedEvent>> _eventCache;
    std::map<uint8, std::vector<ObjectGuid>> _addclassCache;
    std::list<std::pair<farm_zone, std::list<farm_spot>>> _farm_cache_data;
    std::vector<uint32> _currentBots;
    uint32 _playersLevel;
};

#define sRandomPlayerbotMgr RandomPlayerbotMgr::instance()

#endif
