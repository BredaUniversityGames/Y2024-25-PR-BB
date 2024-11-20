#pragma once
#include <engine.hpp>
#include <memory>
#include <scripting_context.hpp>

class ScriptingModule : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override
    {
        ScriptingContext::VMMemoryConfig memory_config {};
        context = std::make_unique<ScriptingContext>(memory_config);
        return ModuleTickOrder::ePreTick;
    };
    void Tick(MAYBE_UNUSED Engine& engine) override {};
    void Shutdown(MAYBE_UNUSED Engine& engine) override {};

public:
    ~ScriptingModule() override = default;

private:
    std::unique_ptr<ScriptingContext> context {};
};
