#pragma once
#include "common.hpp"
#include "module_interface.hpp"

#include <typeindex>
#include <unordered_map>
#include <vector>

// Service locator for all modules
// Instantiate a MainEngine to run the engine, which inherits from this
class Engine
{
public:
    Engine() = default;
    virtual ~Engine() { Reset(); }

    NON_COPYABLE(Engine);
    NON_MOVABLE(Engine);

    template <typename Module>
    Engine& AddModule();

    template <typename Module>
    Module& GetModule();

    template <typename Module>
    Module* GetModuleSafe();

    void SetExit(int exit_code);

protected:
    struct ModulePriorityPair
    {
        ModuleInterface* module;
        ModuleTickOrder priority;
    };

    int _exitCode = 0;
    bool _exitRequested = false;
    std::vector<ModulePriorityPair> _tickOrder {};

    // Cleans up all modules
    void Reset();

private:
    ModuleInterface* GetModuleUntyped(std::type_index type) const;
    void AddModuleToTickList(ModuleInterface* module, ModuleTickOrder priority);
    void RegisterNewModule(std::type_index moduleType, ModuleInterface* module);

    // Raw pointers are used because deallocation order of modules is important

    std::unordered_map<std::type_index, ModuleInterface*> _modules {};
    std::vector<ModuleInterface*> _initOrder {};
};

template <typename Module>
inline Engine& Engine::AddModule()
{
    GetModule<Module>();
    return *this;
}

template <typename Module>
inline Module& Engine::GetModule()
{
    if (auto modulePtr = GetModuleSafe<Module>())
    {
        return static_cast<Module&>(*modulePtr);
    }

    auto type = std::type_index(typeid(Module));
    auto* newModule = new Module();

    RegisterNewModule(type, newModule);
    return *newModule;
}
template <typename Module>
Module* Engine::GetModuleSafe()
{
    return static_cast<Module*>(GetModuleUntyped(std::type_index(typeid(Module))));
}