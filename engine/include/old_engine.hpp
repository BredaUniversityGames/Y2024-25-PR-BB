#pragma once

#include "module_interface.hpp"
#include "performance_tracker.hpp"
#include "mesh.hpp"
#include "particles/particle_interface.hpp"
#include <memory>

class ECS;
class Renderer;
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

private:
    friend Renderer;
    // std::unique_ptr<ThreadPool> _threadPool;
    // std::unique_ptr<AssetManager> _AssetManager;

    std::unique_ptr<Editor> _editor;

    std::unique_ptr<Renderer> _renderer;

    std::unique_ptr<ParticleInterface> _particleInterface;

    std::shared_ptr<ECS> _ecs;

    std::shared_ptr<SceneDescription> _scene;

    glm::ivec2 _lastMousePos {};

    std::chrono::time_point<std::chrono::high_resolution_clock> _lastFrameTime;

    PerformanceTracker _performanceTracker;

    // modules
    std::unique_ptr<PhysicsModule> _physicsModule;

    bool _shouldQuit = false;
};
