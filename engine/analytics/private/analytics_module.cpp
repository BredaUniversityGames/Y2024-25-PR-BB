#include "analytics_module.hpp"

#include "engine.hpp"
#include "file_io.hpp"
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
    auto keyFile = fileIO::OpenReadStream("assets/game_analytics_keys.txt", fileIO::TEXT_READ_FLAGS);
    if (keyFile.has_value())
    {
        std::string key;
        std::string secret;
        std::getline(keyFile.value(), key);
        std::getline(keyFile.value(), secret);

        gameanalytics::GameAnalytics::configureCustomLogHandler(logHandler);
        gameanalytics::GameAnalytics::configureBuild("dev"); // TODO: Formalize this to actual build version
        gameanalytics::GameAnalytics::enableSDKInitEvent(true);

#if DISTRIBUTION
        constexpr bool enabled = false;
#else
        constexpr bool enabled = true;
#endif
        gameanalytics::GameAnalytics::setEnabledErrorReporting(enabled);
        gameanalytics::GameAnalytics::setEnabledEventSubmission(enabled); // TODO: Look into privacy policy later for distribution

        gameanalytics::GameAnalytics::initialize(key, secret);
    }
    else
    {
        bblog::warn("Unable to find key files for analytics, proceeding without analytics!");
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