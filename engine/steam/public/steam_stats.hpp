#pragma once
#include "steam_include.hpp"
#include <cstdint>
#include <string_view>

#define _STAT_ID(id, type, name) { id, type, name, 0, 0, 0, 0 }

enum class EStatTypes
{
    STAT_INT = 0,
    STAT_FLOAT = 1,
    STAT_AVGRATE = 2,
};

struct Stat
{
    Stat(int32_t id, EStatTypes type, std::string_view name)
        : id(id)
        , type(type)
        , name(name)
    {
    }

    Stat() = default;

    int32_t id = 0;
    EStatTypes type = EStatTypes::STAT_INT;
    std::string name = "";
    int value = 0;
    float floatValue = 0.0f;
    float floatAvgNumerator = 0.0f;
    float floatAvgDenominator = 0.0f;
};

class SteamStats
{
private:
    uint64 _appID; // Our current AppID
    std::vector<Stat> _stats;
    std::vector<Stat> _oldStats;
    bool _initialized; // Have we called Request stats and received the callback?

public:
    SteamStats(std::span<Stat> stats);
    ~SteamStats() = default;

    bool StoreStats();
    Stat* GetStat(std::string_view name);

    STEAM_CALLBACK(SteamStats, OnUserStatsReceived, UserStatsReceived_t,
        _callbackUserStatsReceived);
    STEAM_CALLBACK(SteamStats, OnUserStatsStored, UserStatsStored_t,
        _callbackUserStatsStored);
};