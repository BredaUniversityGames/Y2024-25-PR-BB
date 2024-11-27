#include "time_module.hpp"
#include "scripting_module.hpp"

ModuleTickOrder TimeModule::Init(Engine& e)
{
    auto& foreignApi = e.GetModule<ScriptingModule>().GetForeignAPI();

    constexpr auto GetMS = [](TimeModule& self)
    {
        return self.GetDeltatime().count();
    };

    auto& module = foreignApi.klass<TimeModule>("TimeModule");
    module.funcExt<+GetMS>("GetDeltatime");

    return ModuleTickOrder::eFirst;
}

void TimeModule::Tick(MAYBE_UNUSED Engine& e)
{
    current_deltatime = delta_timer.GetElapsed();
    delta_timer.Reset();
    total_time += current_deltatime;
}