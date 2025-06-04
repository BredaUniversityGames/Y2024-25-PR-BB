#include "steam_stats.hpp"
#include "log.hpp"

SteamStats::SteamStats(std::span<Stat> stats)
    : _appID(0)
    , _initialized(false)
    , _callbackUserStatsReceived(this, &SteamStats::OnUserStatsReceived)
    , _callbackUserStatsStored(this, &SteamStats::OnUserStatsStored)
{
    _appID = SteamUtils()->GetAppID();
    _stats.resize(stats.size());
    std::copy(stats.begin(), stats.end(), _stats.begin());
}

bool SteamStats::StoreStats()
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
                SteamUserStats()->SetStat(stat.name.c_str(), stat.value);
                break;

            case EStatTypes::STAT_FLOAT:
                SteamUserStats()->SetStat(stat.name.c_str(), stat.floatValue);
                break;

            case EStatTypes::STAT_AVGRATE:
                SteamUserStats()->UpdateAvgRateStat(stat.name.c_str(), stat.floatAvgNumerator, stat.floatAvgDenominator);
                // The averaged result is calculated for us
                SteamUserStats()->GetStat(stat.name.c_str(), &stat.floatValue);
                break;

            default:
                break;
            }
        }

        return SteamUserStats()->StoreStats();
    }

    return false;
}

void SteamStats::OnUserStatsReceived(UserStatsReceived_t* pCallback)
{
    // we may get callbacks for other games' stats arriving, ignore them
    if (_appID == pCallback->m_nGameID)
    {
        if (k_EResultOK == pCallback->m_eResult)
        {
            bblog::info("Received stats from Steam");
            // load stats
            for (size_t i = 0; i < _stats.size(); ++i)
            {
                Stat& stat = _stats[i];

                // For debug purposes, we can print the stat
                bblog::info("Loaded stat {} with values: {} ; {}. ID: {}", stat.name, stat.value, stat.floatValue, stat.id);
                //
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
            }
            _initialized = true;
        }
        else
        {
            char buffer[128];
            _snprintf(buffer, 128, "RequestStats - failed, %d\n", pCallback->m_eResult);
            bblog::error(buffer);
        }
    }
}

void SteamStats::OnUserStatsStored(UserStatsStored_t* pCallback)
{
    // we may get callbacks for other games' stats arriving, ignore them
    if (_appID == pCallback->m_nGameID)
    {
        if (k_EResultOK == pCallback->m_eResult)
        {
            bblog::info("StoreStats - success\n");
        }
        else if (k_EResultInvalidParam == pCallback->m_eResult)
        {
            // One or more stats we set broke a constraint. They've been reverted,
            // and we should re-iterate the values now to keep in sync.
            bblog::error("StoreStats - some failed to validate\n");
            // Fake up a callback here so that we re-load the values.
            UserStatsReceived_t callback;
            callback.m_eResult = k_EResultOK;
            callback.m_nGameID = _appID;
            OnUserStatsReceived(&callback);
        }
        else
        {
            char buffer[128];
            _snprintf(buffer, 128, "StoreStats - failed, %d\n", pCallback->m_eResult);
            bblog::error(buffer);
        }
    }
}