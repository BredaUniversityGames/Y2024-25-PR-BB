#include "main_engine.hpp"

int MainEngine::Run()
{
    while (!_exit_requested)
    {
        MainLoopOnce();
    }

    return _exit_code;
}
void MainEngine::MainLoopOnce()
{
    for (auto module_priority_pair : _tick_order)
    {
        module_priority_pair.module->Tick(*this);

        if (_exit_requested)
        {
            return;
        }
    }
}

int MainEngine::GetExitCode() const
{
    return _exit_code;
}
