#include "engine.hpp"
#include <algorithm>

void Engine::SetExit(int code)
{
    _exit_requested = true;
    _exit_code = code;
}

void Engine::Reset()
{
    for (auto it = _init_order.rbegin(); it != _init_order.rend(); ++it)
    {
        auto* module = *it;
        module->Shutdown(*this);
        delete module;
    }

    _modules.clear();
    _tick_order.clear();
    _init_order.clear();
    _exit_requested = false;
    _exit_code = 0;
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
void Engine::AddModuleToTickList(ModuleInterface* module, uint32_t priority)
{
    // sorted emplace, based on tick priority

    auto compare = [](const auto& new_elem, const auto& old_elem)
    {
        return new_elem.priority < old_elem.priority;
    };

    auto pair = ModulePriorityPair { module, priority };

    auto insert_it = std::upper_bound(
        _tick_order.begin(), _tick_order.end(), pair, compare);

    _tick_order.insert(insert_it, pair);
}