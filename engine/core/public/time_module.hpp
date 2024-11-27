#pragma once
#include "engine.hpp"
#include "timers.hpp"

class TimeModule : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override
    {
        return ModuleTickOrder::eFirst;
    }

    void Tick(MAYBE_UNUSED Engine& engine) override
    {
        current_deltatime = delta_timer.GetElapsed();
        delta_timer.Reset();
        total_time += current_deltatime;
    }
    void Shutdown(MAYBE_UNUSED Engine& engine) override {};

public:
    NON_COPYABLE(TimeModule);
    NON_MOVABLE(TimeModule);

    TimeModule() = default;
    ~TimeModule() override = default;
    DeltaMS GetDeltatime() const { return current_deltatime; }
    DeltaMS GetTotalTime() const { return total_time; }

private:
    DeltaMS current_deltatime {};
    DeltaMS total_time {};

    Stopwatch delta_timer {};
};