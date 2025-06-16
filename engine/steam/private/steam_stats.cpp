#include "steam_stats.hpp"
#include "log.hpp"

#include <magic_enum.hpp>

SteamStatManager::SteamStatManager(std::span<Stat> stats)
    : _appID(0)
    , _initialized(false)
    , _callbackUserStatsReceived(this, &SteamStatManager::OnUserStatsReceived)
    , _callbackUserStatsStored(this, &SteamStatManager::OnUserStatsStored)
{
    if (auto utils = SteamUtils())
    {
        _appID = utils->GetAppID();

        _stats.resize(stats.size());
        _oldStats.resize(stats.size());

        std::copy(stats.begin(), stats.end(), _stats.begin());
        std::copy(stats.begin(), stats.end(), _oldStats.begin());
    }
}

bool SteamStatManager::StoreStats()
{
    if (_initialized)
    {
        // load stats
        for (size_t i = 0; i < _stats.size(); ++i)
        {
            Stat& stat = _stats[i];
            switch (stat.type)
            {
            case EStatTypes::STAT_INT:
                if (!SteamUserStats()->SetStat(stat.name.c_str(), stat.value))
                {
                    stat.value = _oldStats[i].value;
                }
                break;

            case EStatTypes::STAT_FLOAT:
                if (!SteamUserStats()->SetStat(stat.name.c_str(), stat.floatValue))
                {
                    stat.floatValue = _oldStats[i].floatValue;
                }
                break;

            case EStatTypes::STAT_AVGRATE:
                if (!SteamUserStats()->UpdateAvgRateStat(stat.name.c_str(), stat.floatAvgNumerator, stat.floatAvgDenominator))
                {
                    stat.floatAvgNumerator = _oldStats[i].floatAvgNumerator;
                    stat.floatAvgDenominator = _oldStats[i].floatAvgDenominator;

                    // The averaged result is calculated for us
                    if (!SteamUserStats()->GetStat(stat.name.c_str(), &stat.floatValue))
                    {
                        stat.floatValue = _oldStats[i].floatValue;
                    }
                }
                break;

            default:
                break;
            }
        }

        std::copy(_stats.begin(), _stats.end(), _oldStats.begin());

        return SteamUserStats()->StoreStats();
    }

    return false;
}

std::optional<Stat*> SteamStatManager::GetStat(std::string_view name)
{
    if (!_initialized)
    {
        return std::nullopt;
    }

    auto result = std::find_if(_stats.begin(), _stats.end(), [&name](const auto& val)
        { return val.name == name; });
    if (result == _stats.end())
        return std::nullopt;

    return &*result;
}

void SteamStatManager::OnUserStatsReceived(UserStatsReceived_t* pCallback)
{
    // we may get callbacks for other games' stats arriving, ignore them
    if (_appID == pCallback->m_nGameID)
    {
        if (k_EResultOK == pCallback->m_eResult)
        {
            bblog::info("Received stats from Steam");

            for (size_t i = 0; i < _stats.size(); ++i)
            {
                Stat& stat = _stats[i];

                switch (stat.type)
                {
                case EStatTypes::STAT_INT:
                    SteamUserStats()->GetStat(stat.name.c_str(), &stat.value);
                    break;

                case EStatTypes::STAT_FLOAT:
                case EStatTypes::STAT_AVGRATE:
                    SteamUserStats()->GetStat(stat.name.c_str(), &stat.floatValue);
                    break;

                default:
                    break;
                }
                bblog::info("Loaded stat {} with values: {} ; {}. ID: {}", stat.name, stat.value, stat.floatValue, stat.id);
            }
            _initialized = true;
        }
        else
        {
            bblog::error("RequestStats - failed, {}", magic_enum::enum_name(pCallback->m_eResult));
        }
    }
}

void SteamStatManager::OnUserStatsStored(UserStatsStored_t* pCallback)
{
    // we may get callbacks for other games' stats arriving, ignore them
    if (_appID == pCallback->m_nGameID)
    {
        if (k_EResultOK != pCallback->m_eResult)
        {
            if (k_EResultInvalidParam == pCallback->m_eResult)
            {
                // One or more stats we set broke a constraint. They've been reverted,
                // and we should re-iterate the values now to keep in sync.
                bblog::error("StoreStats - some failed to validate");
                // Fake up a callback here so that we re-load the values.
                UserStatsReceived_t callback;
                callback.m_eResult = k_EResultOK;
                callback.m_nGameID = _appID;
                OnUserStatsReceived(&callback);
            }
            else
            {
                bblog::error("StoreStats - failed, {}", magic_enum::enum_name(pCallback->m_eResult));
            }
        }
    }
}