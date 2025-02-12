#pragma once
#include "common.hpp"
#include "engine.hpp"
#include <memory>
#include <string>

class Editor;
class ImGuiBackend;
class PerformanceTracker;
class ParticleEditor;

class InspectorModule : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Shutdown(Engine& engine) override;
    void Tick(Engine& engine) override;

    std::string_view GetName() override { return "Inspector Module"; }

public:
    InspectorModule();
    ~InspectorModule() override;

    NON_MOVABLE(InspectorModule);
    NON_COPYABLE(InspectorModule);

private:
    std::unique_ptr<Editor> _editor;
    std::unique_ptr<PerformanceTracker> _performanceTracker;
    std::unique_ptr<ParticleEditor> _particleEditor;
    std::shared_ptr<ImGuiBackend> _imguiBackend;

    std::unordered_map<std::string, bool> _openWindows;
};
