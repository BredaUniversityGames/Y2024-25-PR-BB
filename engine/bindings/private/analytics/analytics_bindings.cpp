#include "analytics_bindings.hpp"

#include "utility/enum_bind.hpp"
#include <GameAnalytics/GameAnalytics.h>

namespace bindings
{
using ResourceFlowType = gameanalytics::EGAResourceFlowType;
using ProgressionStatus = gameanalytics::EGAProgressionStatus;
using ErrorSeverity = gameanalytics::EGAErrorSeverity;

class Analytics
{
public:
    static void AddResourceEvent(ResourceFlowType flowType, const std::string& currency, float amount, const std::string& itemType, const std::string& itemId)
    {
        gameanalytics::GameAnalytics::addResourceEvent(flowType, currency, amount, itemType, itemId);
    }
    static void AddDesignEvent(const std::string& eventId, double value)
    {
        gameanalytics::GameAnalytics::addDesignEvent(eventId, value);
    }
    static void AddErrorEvent(ErrorSeverity severity, const std::string& message)
    {
        gameanalytics::GameAnalytics::addErrorEvent(severity, message);
    }
    static void AddProgressionEvent(ProgressionStatus progressionStatus, int32_t score, const std::string& progression01, const std::string& progression02)
    {
        gameanalytics::GameAnalytics::addProgressionEvent(progressionStatus, score, progression01, progression02);
    }
};
}

void BindAnalyticsAPI(wren::ForeignModule& module)
{
    bindings::BindEnum<bindings::ResourceFlowType>(module, "ResourceFlowType");
    bindings::BindEnum<bindings::ProgressionStatus>(module, "ProgressionStatus");
    bindings::BindEnum<bindings::ErrorSeverity>(module, "ErrorSeverity");

    auto& analyticsClass = module.klass<bindings::Analytics>("Analytics");
    analyticsClass.funcStatic<bindings::Analytics::AddResourceEvent>("AddResourceEvent");
    analyticsClass.funcStatic<bindings::Analytics::AddDesignEvent>("AddDesignEvent");
    analyticsClass.funcStatic<bindings::Analytics::AddErrorEvent>("AddErrorEvent");
    analyticsClass.funcStatic<bindings::Analytics::AddProgressionEvent>("AddProgressionEvent");
}
