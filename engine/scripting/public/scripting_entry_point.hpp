#pragma once
#include "common.hpp"

#include "timers.hpp"
#include "utility/module_handle.hpp"

#include <wren.hpp>

class ScriptingEntryPoint
{
public:
    ScriptingEntryPoint() = default;
    ~ScriptingEntryPoint() = default;

    NON_MOVABLE(ScriptingEntryPoint);
    NON_COPYABLE(ScriptingEntryPoint);

    // Sets the main entry point class for Wren
    void SetMainModule(WrenVM* vm, const WrenModuleHandle& module, const std::string& className);
    bool Update(WrenVM* vm, DeltaMS deltatime);

private:
    WrenHandle* _mainClassHandle {};
    WrenHandle* _mainTickMethod {};
};
