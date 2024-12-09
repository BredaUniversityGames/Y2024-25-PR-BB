#include "time_module.hpp"

ModuleTickOrder TimeModule::Init(MAYBE_UNUSED Engine& e)
{
    return ModuleTickOrder::eFirst;
}

void TimeModule::Tick(MAYBE_UNUSED Engine& e)
{
    current_deltatime = delta_timer.GetElapsed();
    delta_timer.Reset();
    total_time += current_deltatime;
}