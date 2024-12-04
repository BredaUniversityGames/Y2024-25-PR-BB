#pragma once
#include "common.hpp"
#include "engine.hpp"

#include "main_script.hpp"
#include "scripting_context.hpp"
#include "utility/wren_engine.hpp"

#include <memory>

class ScriptingModule : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Tick(Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override {};

public:
    NON_COPYABLE(ScriptingModule);
    NON_MOVABLE(ScriptingModule);

    ScriptingModule() = default;
    ~ScriptingModule() override = default;

    // Generates the Engine API file, that contains all foreign definitions
    void GenerateEngineBindingsFile();

    // Sets the main entry point script
    void SetMainScript(Engine& e, const std::string& path);

    wren::ForeignModule& GetForeignAPI() const
    {
        return _context->GetVM().module(_engineBindingsPath);
    }

    ScriptingContext& GetContext() const
    {
        return *_context;
    }

    wren::ForeignKlassImpl<WrenEngine>& GetEngineClass() const
    {
        return GetForeignAPI().klass<WrenEngine>("Engine");
    }

private:
    std::string _engineBindingsPath {};

    std::unique_ptr<ScriptingContext> _context {};
    std::unique_ptr<MainScript> _mainModule {};
};
