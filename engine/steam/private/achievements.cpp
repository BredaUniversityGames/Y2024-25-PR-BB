﻿#include "achievements.hpp"
#include "log.hpp"

#include <magic_enum.hpp>

SteamAchievementManager::SteamAchievementManager(std::span<Achievement> achievements)
    : _appID(0)
    , _initialized(false)
    , _callbackUserStatsReceived(this, &SteamAchievementManager::OnUserStatsReceived)
    , _callbackUserStatsStored(this, &SteamAchievementManager::OnUserStatsStored)
    , _callbackAchievementStored(this, &SteamAchievementManager::OnAchievementStored)
{
    if (auto utils = SteamUtils())
    {
        _appID = utils->GetAppID();
        _achievements.resize(achievements.size());
        std::copy(achievements.begin(), achievements.end(), _achievements.begin());
    }
}

Achievement* SteamAchievementManager::GetAchievement(std::string_view name)
{
    if (!_initialized)
    {
        return nullptr;
    }

    auto result = std::find_if(_achievements.begin(), _achievements.end(), [&name](const auto& val)
        { return val.name == name; });
    if (result == _achievements.end())
        return nullptr;

    return &*result;
}
bool SteamAchievementManager::SetAchievement(const char* ID)
{
    // Have we received a call back from Steam yet?
    if (_initialized)
    {
        SteamUserStats()->SetAchievement(ID);
        return SteamUserStats()->StoreStats();
    }
    // If not then we can't set achievements yet
    return false;
}

void SteamAchievementManager::OnUserStatsReceived(UserStatsReceived_t* pCallback)
{
    // we may get callbacks for other games' stats arriving, ignore them
    if (_appID == pCallback->m_nGameID)
    {
        if (k_EResultOK == pCallback->m_eResult)
        {
            bblog::info("Received achievements from Steam");
            _initialized = true;

            // load achievements
            for (int32_t i = _achievements.size() - 1; i >= 0; --i)
            {
                Achievement& ach = _achievements[i];

                if (SteamUserStats()->GetAchievement(ach.apiId.c_str(), &ach.achieved))
                {
                    std::memcpy(ach.name, SteamUserStats()->GetAchievementDisplayAttribute(ach.apiId.c_str(), "name"), sizeof(ach.name));
                    std::memcpy(ach.description, SteamUserStats()->GetAchievementDisplayAttribute(ach.apiId.c_str(), "desc"), sizeof(ach.description));

                    bblog::info("{}: {}", ach.name, ach.description);
                }
                else
                {
                    _achievements.erase(std::next(_achievements.begin(), i));
                }
            }
        }
        else
        {
            bblog::error("RequestStats - failed, {}", magic_enum::enum_name(pCallback->m_eResult));
        }
    }
}

void SteamAchievementManager::OnUserStatsStored(UserStatsStored_t* pCallback)
{
    // we may get callbacks for other games' stats arriving, ignore them
    if (_appID == pCallback->m_nGameID)
    {
        if (k_EResultOK != pCallback->m_eResult)
        {
            bblog::error("StatsStored - failed, {}", magic_enum::enum_name(pCallback->m_eResult));
        }
    }
}

void SteamAchievementManager::OnAchievementStored(UserAchievementStored_t* pCallback)
{
    // we may get callbacks for other games' stats arriving, ignore them
    if (_appID == pCallback->m_nGameID)
    {
        bblog::info("Stored Achievement for Steam\n");
    }
}