#pragma once

#include "engine_init_info.hpp"
#include "performance_tracker.hpp"
#include "mesh.hpp"
#include "particles/particle_interface.hpp"

class ECS;
class Application;
class Renderer;
class Editor;

class OldEngine
{
public:
    OldEngine(const InitInfo& initInfo, std::shared_ptr<Application> application);
    ~OldEngine();
    NON_COPYABLE(OldEngine);
    NON_MOVABLE(OldEngine);

    void Run();
    bool ShouldQuit() const { return _shouldQuit; };
    void Quit() { _shouldQuit = true; };

private:
    friend Renderer;
    // std::unique_ptr<ThreadPool> _threadPool;
    // std::unique_ptr<AssetManager> _AssetManager;

    std::unique_ptr<Editor> _editor;

    std::unique_ptr<Renderer> _renderer;

    std::unique_ptr<ParticleInterface> _particleInterface;

    std::shared_ptr<ECS> _ecs;

    std::shared_ptr<SceneDescription> _scene;

    std::shared_ptr<Application> _application;

    glm::ivec2 _lastMousePos {};

    std::chrono::time_point<std::chrono::high_resolution_clock> _lastFrameTime;

    PerformanceTracker _performanceTracker;

    bool _shouldQuit = false;
};
