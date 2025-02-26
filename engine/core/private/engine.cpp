#include "engine.hpp"
#include "profile_macros.hpp"

#include <algorithm>

void Engine::SetExit(int code)
{
    _exitRequested = true;
    _exitCode = code;
}

void Engine::Reset()
{
    for (auto it = _initOrder.rbegin(); it != _initOrder.rend(); ++it)
    {
        auto* module = *it;
        module->Shutdown(*this);
        delete module;
    }

    _modules.clear();
    _tickOrder.clear();
    _initOrder.clear();
    _exitRequested = false;
    _exitCode = 0;
}
ModuleInterface* Engine::GetModuleUntyped(std::type_index type) const
{
    if (auto it = _modules.find(type); it != _modules.end())
    {
        return it->second;
    }
    else
    {
        return nullptr;
    }
}
void Engine::AddModuleToTickList(ModuleInterface* module, ModuleTickOrder priority)
{
    // sorted emplace, based on tick priority

    auto compare = [](const auto& new_elem, const auto& old_elem)
    {
        return new_elem.priority < old_elem.priority;
    };

    auto pair = ModulePriorityPair { module, priority };

    auto insertIt = std::upper_bound(
        _tickOrder.begin(), _tickOrder.end(), pair, compare);

    _tickOrder.insert(insertIt, pair);
}

void Engine::RegisterNewModule(std::type_index moduleType, ModuleInterface* module)
{
    ZoneScoped;

    auto name = std::string(module->GetName()) + " init";
    ZoneName(name.c_str(), 32);

    auto [it, success] = _modules.emplace(moduleType, module);
    auto priority = it->second->Init(*this);

    AddModuleToTickList(it->second, priority);
    _initOrder.emplace_back(it->second);
}