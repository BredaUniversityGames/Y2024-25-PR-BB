#pragma once

#include "steam_include.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct Achievement
{
    Achievement(int32_t id, const std::string_view& apiId)
        : id(id)
        , apiId(apiId)
        , name()
        , description()
        , achieved()
        , iconImage()
    {
    }
    Achievement() = default;

    int32_t id = 0;
    std::string apiId = "";
    char name[128];
    char description[256];
    bool achieved = false;
    int32_t iconImage = 0;
};

class SteamAchievements
{
private:
    uint64_t _appID; // Our current AppID
    std::vector<Achievement> _achievements;
    bool _initialized; // Have we called Request stats and received the callback?

public:
    SteamAchievements(std::span<Achievement> achievements);
    ~SteamAchievements() = default;

    Achievement* GetAchievement(std::string_view name);

    bool SetAchievement(const char* ID);

    STEAM_CALLBACK(SteamAchievements, OnUserStatsReceived, UserStatsReceived_t,
        _callbackUserStatsReceived);
    STEAM_CALLBACK(SteamAchievements, OnUserStatsStored, UserStatsStored_t,
        _callbackUserStatsStored);
    STEAM_CALLBACK(SteamAchievements, OnAchievementStored,
        UserAchievementStored_t, _callbackAchievementStored);
};