#include "time_module.hpp"

ModuleTickOrder TimeModule::Init(MAYBE_UNUSED Engine& e)
{
    return ModuleTickOrder::eFirst;
}

void TimeModule::Tick(MAYBE_UNUSED Engine& e)
{
    _currentDeltaTime = _deltaTimer.GetElapsed() * _deltaTimeScale;
    _deltaTimer.Reset();
    _totalTime += _currentDeltaTime;
}