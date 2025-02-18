#include "RandomPlayerbotBracketMgr.h"

#include "Helper.h"
#include "PlayerbotSpec.h"
#include "RandomPlayerbotMgr.h"

RandomBotBacketManager::RandomBotBacketManager()
    : _timer{ 0 }
    , _BotDistDebugMode{ true }
{
    _LevelRanges[0]     = { 1, 9,   2 };
    _LevelRanges[1]     = { 10, 19, 8 };
    _LevelRanges[2]     = { 20, 29, 8 };
    _LevelRanges[3]     = { 30, 39, 8 };
    _LevelRanges[4]     = { 40, 49, 8 };
    _LevelRanges[5]     = { 50, 59, 9 };
    _LevelRanges[6]     = { 60, 69, 9 };
    _LevelRanges[7]     = { 70, 79, 9 };
    _LevelRanges[8]     = { 80, 84, 9 };
    _LevelRanges[9]     = { 85, 89, 10 };
    _LevelRanges[10]    = { 90, 90, 20 };

    uint32 totalPercent = 0;
    for (uint8 i = 0; i < NUM_RANGES; ++i)
        totalPercent += _LevelRanges[i].desiredPercent;
    if (totalPercent != 100)
        TC_LOG_ERROR("server.loading", "[BotLevelBrackets] Sum of percentages is %u (expected 100).", totalPercent);
}

int RandomBotBacketManager::GetLevelRangeIndex(uint8 level)
{
    for (int i = 0; i < NUM_RANGES; ++i)
    {
        if (level >= _LevelRanges[i].lower && level <= _LevelRanges[i].upper)
            return i;
    }
    return -1;
}

uint8 RandomBotBacketManager::GetRandomLevelInRange(const LevelRangeConfig& range)
{
    return urand(range.lower, range.upper);
}

void RandomBotBacketManager::AdjustBotToRange(Player* bot, int targetRangeIndex)
{
    if (!bot || targetRangeIndex < 0 || targetRangeIndex >= NUM_RANGES)
        return;

    uint8 botOriginalLevel = bot->GetLevel();

    uint8 newLevel = 0;
    // If the bot is a Death Knight, ensure level is not set below 55.
    if (bot->GetClass() == CLASS_DEATH_KNIGHT)
    {
        uint8 lowerBound = _LevelRanges[targetRangeIndex].lower;
        uint8 upperBound = _LevelRanges[targetRangeIndex].upper;
        if (upperBound < 55)
        {
            // This target range is invalid for Death Knights.
            if (_BotDistDebugMode)
            {
                //TC_LOG_DEBUG("playerbots", "[BotLevelBrackets] AdjustBotToRange: Cannot assign Death Knight '%s' (%u) to range %u-%u (below level 55).", bot->GetName().c_str(), botOriginalLevel, lowerBound, upperBound);
            }
            return;
        }
        // Adjust lower bound to 55 if necessary.
        if (lowerBound < 55)
            lowerBound = 55;
        newLevel = urand(lowerBound, upperBound);
    }
    else
    {
        newLevel = GetRandomLevelInRange(_LevelRanges[targetRangeIndex]);
    }

    sRandomPlayerbotMgr->TagForRandomize(bot, newLevel);

    if (_BotDistDebugMode)
    {
        std::string playerClassName = bot ? ClassToString((Classes)bot->GetClass()) : "Unknown";
        //TC_LOG_DEBUG("playerbots", "[BotLevelBrackets] AdjustBotToRange: Bot '%s' - %s (%u) adjusted to level %u (target range %u-%u).", bot->GetName().c_str(), playerClassName.c_str(), botOriginalLevel, newLevel, _LevelRanges[targetRangeIndex].lower, _LevelRanges[targetRangeIndex].upper);
    }
}

void RandomBotBacketManager::Update(uint32 diff)
{
    _timer += diff;
    if (_timer < (_BotDistCheckFrequency * 1000))
        return;
    _timer = 0;

    // Build the current distribution for bots.
    uint32 totalBots = 0;
    int actualCounts[NUM_RANGES] = { 0 };
    std::vector<Player*> botsByRange[NUM_RANGES];

    auto const& allPlayers = ObjectAccessor::GetPlayers();
    for (auto const& itr : allPlayers)
    {
        Player* player = itr.second;
        if (!player || !player->IsInWorld())
            continue;
        
        if (!sRandomPlayerbotMgr->IsRandomBot(player->GetGUID().GetCounter()))
            continue;

        if (player->InBattleground() || player->InBattlegroundQueue() || player->inRandomLfgDungeon() || player->GetInstanceId())
            continue;

        totalBots++;
        int rangeIndex = GetLevelRangeIndex(player->GetLevel());
        if (rangeIndex >= 0)
        {
            actualCounts[rangeIndex]++;
            botsByRange[rangeIndex].push_back(player);
        }
        else if (_BotDistDebugMode)
        {
            //TC_LOG_DEBUG("playerbots", "[BotLevelBrackets] Bot '%s' with level %u does not fall into any defined range.", player->GetName().c_str(), player->GetLevel());
        }
    }

    if (totalBots == 0)
        return;

    // Compute the desired count for each range.
    int desiredCounts[NUM_RANGES] = { 0 };
    for (int i = 0; i < NUM_RANGES; ++i)
    {
        desiredCounts[i] = static_cast<int>(round((_LevelRanges[i].desiredPercent / 100.0) * totalBots));
        if (_BotDistDebugMode)
        {
            TC_LOG_INFO("playerbots", "[BotLevelBrackets] Range %u (%u-%u): Desired = %u, Actual = %u.", i + 1, _LevelRanges[i].lower, _LevelRanges[i].upper, desiredCounts[i], actualCounts[i]);
        }
    }

    // For each range that has a surplus, reassign bots to ranges that are underpopulated.
    for (int i = 0; i < NUM_RANGES; ++i)
    {
        while (actualCounts[i] > desiredCounts[i] && !botsByRange[i].empty())
        {
            Player* bot = botsByRange[i].back();
            botsByRange[i].pop_back();

            int targetRange = -1;
            // For Death Knights, only consider target ranges where the upper bound is at least 55.
            if (bot->GetClass() == CLASS_DEATH_KNIGHT)
            {
                for (int j = 0; j < NUM_RANGES; ++j)
                {
                    if (actualCounts[j] < desiredCounts[j] && _LevelRanges[j].upper >= 55)
                    {
                        targetRange = j;
                        break;
                    }
                }
            }
            else
            {
                for (int j = 0; j < NUM_RANGES; ++j)
                {
                    if (actualCounts[j] < desiredCounts[j])
                    {
                        targetRange = j;
                        break;
                    }
                }
            }

            if (targetRange == -1)
                break; // No appropriate underpopulated range found.

            AdjustBotToRange(bot, targetRange);
            actualCounts[i]--;
            actualCounts[targetRange]++;
        }
    }

    if (_BotDistDebugMode)
        TC_LOG_DEBUG("playerbots", "[BotLevelBrackets] Distribution adjustment complete. Total bots: %u.", totalBots);
}