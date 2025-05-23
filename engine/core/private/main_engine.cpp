#include "main_engine.hpp"

#include <string>
#include <tracy/Tracy.hpp>

int MainEngine::Run()
{
    while (!_exitRequested)
    {
        MainLoopOnce();
        FrameMark;
    }

    return _exitCode;
}
void MainEngine::MainLoopOnce()
{
    ZoneScoped;
    for (auto modulePriorityPair : _tickOrder)
    {
        ZoneScoped;

        auto name = std::string(modulePriorityPair.module->GetName()) + " tick";
        ZoneName(name.c_str(), 32);

        modulePriorityPair.module->Tick(*this);

        if (_exitRequested)
        {
            return;
        }
    }
}

int MainEngine::GetExitCode() const
{
    return _exitCode;
}
