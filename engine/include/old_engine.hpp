#pragma once

#include "mesh.hpp"
#include "module_interface.hpp"
#include "particle_interface.hpp"
#include "performance_tracker.hpp"

#include <memory>

struct RigidbodyComponent;
class ECS;
class Renderer;
class Editor;
class PhysicsModule;
class OldEngine : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Tick(Engine& engine) override;
    void Shutdown(Engine& engine) override;

public:
    OldEngine();
    ~OldEngine() override;

    std::shared_ptr<ECS> GetECS() const { return _ecs; }

private:
    // std::unique_ptr<ThreadPool> _threadPool;
    // std::unique_ptr<AssetManager> _AssetManager;

    std::unique_ptr<Editor> _editor;
    std::shared_ptr<ECS> _ecs;
    glm::ivec2 _lastMousePos {};
    std::chrono::time_point<std::chrono::high_resolution_clock> _lastFrameTime;
    PerformanceTracker _performanceTracker;

    // modules
    std::unique_ptr<PhysicsModule> _physicsModule;

    MAYBE_UNUSED bool _shouldQuit = false;
};
