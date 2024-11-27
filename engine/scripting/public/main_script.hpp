#pragma once
#include "common.hpp"
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
    bool Update(DeltaMS deltatime);

private:
    bool valid = false;
    wren::Variable mainClass {};
    wren::Method mainUpdate {};
};
