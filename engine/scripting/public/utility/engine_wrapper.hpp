#pragma once
#include "engine.hpp"
#include "wren_common.hpp"

// Wrapper class for Accessing the engine
struct EngineWrapper
{
    Engine* instance {};

    template <typename T>
    std::optional<T*> GetModule()
    {
        if (instance == nullptr)
            return std::nullopt;

        if (auto ptr = instance->GetModuleSafe<T>())
            return ptr;

        return std::nullopt;
    }

    template <typename T>
    static void BindModule(wren::ForeignKlassImpl<WrenEngine>& engineClass, const std::string& name)
    {
        engineClass.func<&WrenEngine::GetModule<T>>("Get" + name);
    }
};