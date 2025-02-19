#pragma once
#include "common.hpp"
#include "engine.hpp"
#include "timers.hpp"
#include "wren_common.hpp"

class MainScript
{
public:
    // Sets the main execution script for Wren
    // This script must define a "Main" class with the following static methods:
    // static Start(engine) -> void
    // static Update(engine, dt) -> void
    // static Shutdown(engine) -> void

    MainScript(
        Engine* engine,
        wren::VM& vm,
        const std::string& module,
        const std::string& className);

    ~MainScript();

    NON_MOVABLE(MainScript);
    NON_COPYABLE(MainScript);

    // Calls the scripts main update function
    void Update(DeltaMS deltatime);

    // Checks if the script can be run without running into an error
    // Either the script is not set or the previous run threw an exception
    bool IsValid() const { return valid; }

private:
    Engine* engine {};

    bool valid = false;
    wren::Variable mainClass {};
    wren::Method mainUpdate {};
    wren::Method mainInit {};
    wren::Method mainShutdown {};
};
