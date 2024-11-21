#include "main_engine.hpp"
#include <log.hpp>

int MainEngine::Run()
{
    if (_tickOrder.empty())
    {
        bblog::warn("No modules registered, Engine will return immediately");
        return 0;
    }

    while (!_exitRequested)
    {
        MainLoopOnce();
    }

    return _exitCode;
}
void MainEngine::MainLoopOnce()
{
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
