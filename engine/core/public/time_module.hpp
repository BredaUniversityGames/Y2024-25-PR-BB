#pragma once
#include "engine.hpp"
#include "timers.hpp"

class TimeModule : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override;
    void Tick(MAYBE_UNUSED Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override {};
    std::string_view GetName() override { return "Time Module"; }

public:
    NON_COPYABLE(TimeModule);
    NON_MOVABLE(TimeModule);

    TimeModule() = default;
    ~TimeModule() override = default;
    DeltaMS GetDeltatime() const { return current_deltatime; }
    DeltaMS GetTotalTime() const { return total_time; }
    void ResetTimer()
    {
        delta_timer.Reset();
        current_deltatime = {};
    }

private:
    DeltaMS current_deltatime {};
    DeltaMS total_time {};

    Stopwatch delta_timer {};
};