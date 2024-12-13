#include "main_engine.hpp"

#include <tracy/Tracy.hpp>

int MainEngine::Run()
{
    while (!_exitRequested)
    {
        MainLoopOnce();
    }

    return _exitCode;
}
void MainEngine::MainLoopOnce()
{
    ZoneScoped;
    for (auto modulePriorityPair : _tickOrder)
    {
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
