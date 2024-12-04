#include "time_module.hpp"
#include "scripting_module.hpp"

namespace bind
{

float TimeModuleGetDeltatime(TimeModule& self)
{
    return self.GetDeltatime().count();
}

}

ModuleTickOrder TimeModule::Init(Engine& e)
{
    auto& foreignApi = e.GetModule<ScriptingModule>().GetForeignAPI();

    auto& module = foreignApi.klass<TimeModule>("TimeModule");
    module.funcExt<bind::TimeModuleGetDeltatime>("GetDeltatime");

    return ModuleTickOrder::eFirst;
}

void TimeModule::Tick(MAYBE_UNUSED Engine& e)
{
    current_deltatime = delta_timer.GetElapsed();
    delta_timer.Reset();
    total_time += current_deltatime;
}