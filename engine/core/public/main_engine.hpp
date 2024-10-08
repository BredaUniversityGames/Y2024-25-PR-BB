#pragma once
#include "engine.hpp"

// Engine subclass meant to run and exit the application
class MainEngine : public Engine
{
public:
    virtual ~MainEngine() = default;

    // Runs the engine, returns exit code
    int Run();

    // Executes tick loop once
    void MainLoopOnce();

    // Returns 0 if the exit code is not set
    int GetExitCode() const;

    // Resets the engine, can be run again
    void Reset() { Engine::Reset(); };
};