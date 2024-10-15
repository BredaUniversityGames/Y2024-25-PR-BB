#pragma once

#include "module_interface.hpp"
#include "performance_tracker.hpp"
#include "mesh.hpp"

#include <memory>

class ECS;
class Renderer;
class Editor;

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

    std::unique_ptr<ECS> _ecs;

    std::shared_ptr<SceneDescription> _scene;

    glm::ivec2 _lastMousePos {};

    std::chrono::time_point<std::chrono::high_resolution_clock> _lastFrameTime;

    PerformanceTracker _performanceTracker;

    bool _shouldQuit = false;
};
