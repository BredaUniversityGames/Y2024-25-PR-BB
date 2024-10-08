#pragma once
#include "fwd_core.hpp"
#include "module_interface.hpp"
#include "class_decorations.hpp"

#include <memory>
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
        uint32_t priority;
    };

    int _exit_code = 0;
    bool _exit_requested = false;
    std::vector<ModulePriorityPair> _tick_order {};

    // Cleans up all modules
    void Reset();

private:
    ModuleInterface* GetModuleUntyped(std::type_index type) const;
    void AddModuleToTickList(ModuleInterface* module, uint32_t priority);

    // Raw pointers are used because deallocation order of modules is important

    std::unordered_map<std::type_index, ModuleInterface*> _modules {};
    std::vector<ModuleInterface*> _init_order {};
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
    if (auto module_ptr = GetModuleSafe<Module>())
    {
        return static_cast<Module&>(*module_ptr);
    }

    auto type = std::type_index(typeid(Module));
    auto [it, success] = _modules.emplace(type, new Module());
    auto priority = it->second->Init(*this);

    AddModuleToTickList(it->second, priority);
    _init_order.emplace_back(it->second);

    return static_cast<Module&>(*it->second);
}
template <typename Module>
Module* Engine::GetModuleSafe()
{
    return static_cast<Module*>(GetModuleUntyped(std::type_index(typeid(Module))));
}