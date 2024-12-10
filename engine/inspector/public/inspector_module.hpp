#pragma once
#include "common.hpp"
#include "engine.hpp"

class InspectorModule : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Shutdown(Engine& engine) override;
    void Tick(Engine& engine) override;

public:
    InspectorModule() = default;
    ~InspectorModule() override = default;

    NON_MOVABLE(InspectorModule);
    NON_COPYABLE(InspectorModule);
};
