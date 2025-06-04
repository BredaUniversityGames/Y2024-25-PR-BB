#pragma once
#include "steam_include.hpp"
#include <cstdint>

#define _STAT_ID( id,type,name ) { id, type, name, 0, 0, 0, 0 }

enum EStatTypes
{
    STAT_INT = 0,
    STAT_FLOAT = 1,
    STAT_AVGRATE = 2,
};

struct Stat_t
{
    int m_ID;
    EStatTypes m_eStatType;
    const char *m_pchStatName;
    int m_iValue;
    float m_flValue;
    float m_flAvgNumerator;
    float m_flAvgDenominator;
};

class CSteamStats
{
private:
    int64 m_iAppID; // Our current AppID
    Stat_t *m_pStats; // Stats data
    int m_iNumStats; // The number of Stats
    bool m_bInitialized; // Have we called Request stats and received the callback?

public:
    CSteamStats(Stat_t *Stats, int NumStats);
    ~CSteamStats();

    bool RequestStats();
    bool StoreStats();

    STEAM_CALLBACK( CSteamStats, OnUserStatsReceived, UserStatsReceived_t,
            m_CallbackUserStatsReceived );
    STEAM_CALLBACK( CSteamStats, OnUserStatsStored, UserStatsStored_t,
            m_CallbackUserStatsStored );
};