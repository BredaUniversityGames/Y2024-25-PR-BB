#pragma once
#include <engine.hpp>
#include <memory>
#include <scripting_context.hpp>

class ScriptingModule : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override
    {
        ScriptingContext::VMMemoryConfig memory_config {};
        context = std::make_unique<ScriptingContext>(memory_config);
        return ModuleTickOrder::ePreTick;
    };
    void Tick(Engine& engine) override {};
    void Shutdown(Engine& engine) override {};

public:
    ~ScriptingModule() override = default;

private:
    std::unique_ptr<ScriptingContext> context {};
};
