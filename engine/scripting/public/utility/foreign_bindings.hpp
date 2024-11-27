#pragma once
#include "engine.hpp"
#include "wren_common.hpp"

#include <ostream>

struct ForeignBindings
{
    ForeignBindings(const std::string& engineModule);

    std::string GetModuleName() const { return _engineModule; }
    wren::ForeignModule& GetForeignModule(wren::VM& vm) const { return vm.module(_engineModule); };

    void WriteToStream(wren::VM& vm, std::ostream& output) const;

private:
    std::string _engineModule;
};

// Wrapper class for Accessing the engine
struct WrenEngine
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

    void SetEngine(const WrenEngine& v);

    template <typename T>
    static void BindModule(wren::ForeignKlassImpl<WrenEngine>& engineClass, const std::string& name)
    {
        engineClass.func<&WrenEngine::GetModule<T>>("Get" + name);
    }

    static wren::ForeignKlassImpl<WrenEngine>& BindEngineStart(wren::ForeignModule& api);
    static void BindEngineEnd(wren::ForeignModule& api, wren::VM& vm, Engine& e);
};