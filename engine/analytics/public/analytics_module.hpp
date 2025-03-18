#pragma once

#include "module_interface.hpp"

class AnalyticsModule : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Tick(MAYBE_UNUSED Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override;
    std::string_view GetName() override { return "Analytics Module"; }

public:
    AnalyticsModule() = default;
    ~AnalyticsModule() override = default;

    NON_COPYABLE(AnalyticsModule);
    NON_MOVABLE(AnalyticsModule);
};
