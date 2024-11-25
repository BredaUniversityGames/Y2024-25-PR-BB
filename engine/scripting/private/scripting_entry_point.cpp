#include "scripting_entry_point.hpp"
#include <wren.hpp>

void ScriptingEntryPoint::SetMainModule(WrenVM* vm, const WrenModuleHandle& module, const std::string& className)
{
    wrenEnsureSlots(vm, 1);
    wrenGetVariable(vm, module.moduleName.c_str(), className.c_str(), 0);

    _mainClassHandle = wrenGetSlotHandle(vm, 0);
    _mainTickMethod = wrenMakeCallHandle(vm, "Update(_)");
}

bool ScriptingEntryPoint::Update(WrenVM* vm, DeltaMS deltatime)
{
    wrenEnsureSlots(vm, 2);
    wrenSetSlotHandle(vm, 0, _mainClassHandle);

    double casted = static_cast<double>(deltatime.count());
    wrenSetSlotDouble(vm, 1, casted);

    auto result = wrenCall(vm, _mainTickMethod);
    return result == WREN_RESULT_SUCCESS;
}