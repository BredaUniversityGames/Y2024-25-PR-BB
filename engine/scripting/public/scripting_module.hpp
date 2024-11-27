#pragma once
#include "common.hpp"

#include "engine.hpp"
#include "main_script.hpp"
#include "scripting_context.hpp"

#include <memory>

class ScriptingModule : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override;
    void Tick(MAYBE_UNUSED Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override {};

public:
    NON_COPYABLE(ScriptingModule);
    NON_MOVABLE(ScriptingModule);

    ScriptingModule() = default;
    ~ScriptingModule() override = default;

    // Generates the Engine API file, that contains all the foreign definitions
    void GenerateEngineBindingsFile(const std::string& path);

    // Sets the main execution script for Wren
    // This script must define a "Main" class with the following static methods:
    // static Start() -> void
    // static Update(dt) -> void
    void SetMainScript(const std::string& path);

    ScriptingContext& GetContext() const
    {
        return *_context;
    }

    wren::ForeignModule& GetForeignAPI() const
    {
        return _foreign_bindings.GetForeignModule(_context->GetVM());
    }

    auto& StartEngineBind() const
    {
        return WrenEngine::BindEngineStart(GetForeignAPI());
    }

    // Module will be visible from wren
    // Exposes a .Get*name*() function in the Engine global object
    template <typename T>
    void BindModule(wren::ForeignKlassImpl<WrenEngine>& e, const std::string& name)
    {
        WrenEngine::BindModule<T>(e, name);
    }

    void EndEngineBind(Engine& e) const
    {
        WrenEngine::BindEngineEnd(GetForeignAPI(), _context->GetVM(), e);
    }

private:
    ForeignBindings _foreign_bindings { "EngineAPI" };

    std::unique_ptr<ScriptingContext> _context {};
    std::unique_ptr<MainScript> _mainModule {};
};
