#pragma once

#include "module_interface.hpp"
#include "performance_tracker.hpp"
#include "renderer_public.hpp"

#include <memory>

class ECS;
class Editor;
class PhysicsModule;
class OldEngine : public ModuleInterface
{
    virtual ModuleTickOrder Init(Engine& engine) override;
    virtual void Tick(Engine& engine) override;
    virtual void Shutdown(Engine& engine) override;

public:
    OldEngine();
    ~OldEngine() override;

    std::shared_ptr<ECS> GetECS() { return _ecs; }
    float DeltaTimeMS() const { return _deltaTimeMS; };

private:
    // std::unique_ptr<ThreadPool> _threadPool;
    // std::unique_ptr<AssetManager> _AssetManager;

    float _deltaTimeMS;

    std::unique_ptr<Editor> _editor;

    std::shared_ptr<ECS> _ecs;

    std::shared_ptr<SceneDescription> _scene;

    glm::ivec2 _lastMousePos {};

    std::chrono::time_point<std::chrono::high_resolution_clock> _lastFrameTime;

    PerformanceTracker _performanceTracker;

    std::unique_ptr<PhysicsModule> _physicsModule;

    bool _shouldQuit = false;
};
