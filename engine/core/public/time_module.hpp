#pragma once
#include "engine.hpp"
#include "timers.hpp"

class TimeModule : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override;
    void Tick(MAYBE_UNUSED Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override { };
    std::string_view GetName() override { return "Time Module"; }

public:
    NON_COPYABLE(TimeModule);
    NON_MOVABLE(TimeModule);

    TimeModule() = default;
    ~TimeModule() override = default;
    DeltaMS GetDeltatime() const { return _currentDeltaTime; }
    DeltaMS GetTotalTime() const { return _totalTime; }

    void SetDeltatimeScale(float scale)
    {
        _deltaTimeScale = scale;
    }

    void ResetTimer()
    {
        _deltaTimer.Reset();
        _currentDeltaTime = {};
    }

private:
    float _deltaTimeScale = 1.0f;

    DeltaMS _currentDeltaTime {};
    DeltaMS _totalTime {};

    Stopwatch _deltaTimer {};
};