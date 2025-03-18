#include "analytics_module.hpp"

#include "engine.hpp"
#include "log.hpp"

#include <GameAnalytics/GameAnalytics.h>
#include <fstream>

void logHandler(const std::string& message, gameanalytics::EGALoggerMessageType type)
{
    switch (type)
    {
    case gameanalytics::LogError:
        bblog::error(message);
        break;
    case gameanalytics::LogWarning:
        bblog::warn(message);
        break;
    case gameanalytics::LogInfo:
        bblog::info(message);
        break;
    case gameanalytics::LogDebug:
        bblog::debug(message);
        break;
    case gameanalytics::LogVerbose:
        bblog::info(message);
        break;
    }
}

ModuleTickOrder AnalyticsModule::Init(MAYBE_UNUSED Engine& engine)
{
    if (std::ifstream keyFile { "game_analytics_keys.txt" })
    {
        std::string key;
        std::string secret;
        std::getline(keyFile, key);
        std::getline(keyFile, secret);

        gameanalytics::GameAnalytics::configureCustomLogHandler(logHandler);
        gameanalytics::GameAnalytics::configureBuild("dev"); // TODO: Formalize this to actual build version
        gameanalytics::GameAnalytics::enableSDKInitEvent(true);

        gameanalytics::GameAnalytics::setEnabledErrorReporting(true);
        gameanalytics::GameAnalytics::setEnabledEventSubmission(true); // TODO: Look into privacy policy later for distribution

        gameanalytics::GameAnalytics::initialize(key, secret);
    }

    return ModuleTickOrder::eLast;
}

void AnalyticsModule::Tick(MAYBE_UNUSED Engine& engine)
{
}

void AnalyticsModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    gameanalytics::GameAnalytics::onQuit();
}