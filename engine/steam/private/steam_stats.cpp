#include "steam_stats.hpp"
#include "log.hpp"

CSteamStats::CSteamStats(Stat_t* Stats, int NumStats)
    : m_iAppID(0)
    , m_bInitialized(false)
    , m_CallbackUserStatsReceived(this, &CSteamStats::OnUserStatsReceived)
    , m_CallbackUserStatsStored(this, &CSteamStats::OnUserStatsStored)
{
    m_iAppID = SteamUtils()->GetAppID();
    m_pStats = Stats;
    m_iNumStats = NumStats;
    RequestStats();
}

bool CSteamStats::RequestStats()
{
    // Is Steam loaded? If not we can't get stats.
    if (NULL == SteamUserStats() || NULL == SteamUser())
    {
        return false;
    }
    // Is the user logged on?  If not we can't get stats.
    if (!SteamUser()->BLoggedOn())
    {
        return false;
    }
    // Request user stats.
    return SteamUserStats()->RequestCurrentStats();
}

bool CSteamStats::StoreStats()
{
    if (m_bInitialized)
    {
        // load stats
        for (int iStat = 0; iStat < m_iNumStats; ++iStat)
        {
            Stat_t& stat = m_pStats[iStat];
            switch (stat.m_eStatType)
            {
            case STAT_INT:
                SteamUserStats()->SetStat(stat.m_pchStatName, stat.m_iValue);
                break;

            case STAT_FLOAT:
                SteamUserStats()->SetStat(stat.m_pchStatName, stat.m_flValue);
                break;

            case STAT_AVGRATE:
                SteamUserStats()->UpdateAvgRateStat(stat.m_pchStatName, stat.m_flAvgNumerator, stat.m_flAvgDenominator);
                // The averaged result is calculated for us
                SteamUserStats()->GetStat(stat.m_pchStatName, &stat.m_flValue);
                break;

            default:
                break;
            }
        }

        return SteamUserStats()->StoreStats();
    }
}

void CSteamStats::OnUserStatsReceived(UserStatsReceived_t* pCallback)
{
    // we may get callbacks for other games' stats arriving, ignore them
    if (m_iAppID == pCallback->m_nGameID)
    {
        if (k_EResultOK == pCallback->m_eResult)
        {
            bblog::info("Received stats from Steam\n");
            // load stats
            for (int iStat = 0; iStat < m_iNumStats; ++iStat)
            {
                Stat_t& stat = m_pStats[iStat];

                // For debug purposes, we can print the stat
                bblog::info("Loaded stat {} with values: {} ; {}. ID: {}", stat.m_pchStatName, stat.m_iValue, stat.m_flValue, stat.m_ID);
                //
                switch (stat.m_eStatType)
                {
                case STAT_INT:
                    SteamUserStats()->GetStat(stat.m_pchStatName, &stat.m_iValue);
                    break;

                case STAT_FLOAT:
                case STAT_AVGRATE:
                    SteamUserStats()->GetStat(stat.m_pchStatName, &stat.m_flValue);
                    break;

                default:
                    break;
                }
            }
            m_bInitialized = true;
        }
        else
        {
            char buffer[128];
            _snprintf(buffer, 128, "RequestStats - failed, %d\n", pCallback->m_eResult);
            bblog::error(buffer);
        }
    }
}

void CSteamStats::OnUserStatsStored(UserStatsStored_t* pCallback)
{
    // we may get callbacks for other games' stats arriving, ignore them
    if (m_iAppID == pCallback->m_nGameID)
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
            callback.m_nGameID = m_iAppID;
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