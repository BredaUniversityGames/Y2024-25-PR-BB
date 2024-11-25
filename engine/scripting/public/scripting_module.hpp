#pragma once
#include "common.hpp"

#include "scripting_context.hpp"
#include "scripting_entry_point.hpp"
#include "engine.hpp"

#include <memory>

class ScriptingModule : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override
    {
        ScriptingContext::VMMemoryConfig memory_config {};
        _context = std::make_unique<ScriptingContext>(memory_config);
        _mainModule = std::make_unique<ScriptingEntryPoint>();
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
    std::unique_ptr<ScriptingEntryPoint> _mainModule {};
};
