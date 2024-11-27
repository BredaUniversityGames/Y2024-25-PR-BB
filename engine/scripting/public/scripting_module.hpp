#pragma once
#include "common.hpp"

#include "engine.hpp"
#include "main_script.hpp"
#include "scripting_context.hpp"

#include <memory>

class ScriptingModule : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override
    {
        ScriptingContext::VMInitConfig config {};
        config.includePaths.emplace_back("./");
        config.includePaths.emplace_back("./game/");

        _context = std::make_unique<ScriptingContext>(config);

        _mainModule = std::make_unique<MainScript>();

        auto result = _context->InterpretWrenModule("Main.wren");

        if (result)
        {
            _mainModule->SetMainModule(_context->GetVM(), result.value(), "Main");
        }

        return ModuleTickOrder::ePreTick;
    };

    void Tick(MAYBE_UNUSED Engine& engine) override {};
    void Shutdown(MAYBE_UNUSED Engine& engine) override {};

public:
    NON_COPYABLE(ScriptingModule);
    NON_MOVABLE(ScriptingModule);

    ~ScriptingModule() override = default;

private:
    std::unique_ptr<ScriptingContext> _context {};
    std::unique_ptr<MainScript> _mainModule {};
};
