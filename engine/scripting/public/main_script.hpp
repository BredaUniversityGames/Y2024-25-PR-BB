#pragma once
#include "common.hpp"
#include "engine.hpp"
#include "timers.hpp"
#include "wren_common.hpp"

class MainScript
{
public:
    MainScript() = default;
    ~MainScript() = default;

    NON_MOVABLE(MainScript);
    NON_COPYABLE(MainScript);

    // Sets the main entry point class for Wren
    void SetMainModule(wren::VM& vm, const std::string& module, const std::string& className);

    // Initializes the main entry point class for Wren
    void InitMainModule(Engine& e);

    // Calls the script main update function
    void Update(DeltaMS deltatime);

    // Checks if the script can be run without running into an error
    // Either the script is not set or the previous run threw an exception
    bool IsValid() const { return valid; }

private:
    bool valid = false;
    wren::Variable mainClass {};
    wren::Method mainUpdate {};
    wren::Method mainInit {};
};
