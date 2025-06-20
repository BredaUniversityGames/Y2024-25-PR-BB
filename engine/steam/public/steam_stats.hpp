﻿#pragma once
#include "steam_include.hpp"
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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

class SteamStatManager
{
private:
    uint64 _appID; // Our current AppID
    std::vector<Stat> _stats;
    std::vector<Stat> _oldStats;
    bool _initialized; // Have we called Request stats and received the callback?

public:
    SteamStatManager(std::span<Stat> stats);
    ~SteamStatManager() = default;

    bool StoreStats();
    std::optional<Stat*> GetStat(std::string_view name);

    STEAM_CALLBACK(SteamStatManager, OnUserStatsReceived, UserStatsReceived_t,
        _callbackUserStatsReceived);
    STEAM_CALLBACK(SteamStatManager, OnUserStatsStored, UserStatsStored_t,
        _callbackUserStatsStored);
};