#pragma once

#include "swap_chain.hpp"
#include "engine_init_info.hpp"
#include "performance_tracker.hpp"
#include "mesh.hpp"
#include "camera.hpp"

class Application;
class GeometryPipeline;
class LightingPipeline;
class SkydomePipeline;
class TonemappingPipeline;
class IBLPipeline;
class SwapChain;
class GBuffers;
class VulkanBrain;
class ModelLoader;
class Renderer;
class Editor;

class Engine
{
public:
    Engine(const InitInfo& initInfo, std::shared_ptr<Application> application);
    ~Engine();
    NON_COPYABLE(Engine);
    NON_MOVABLE(Engine);

    void Run();
    bool ShouldQuit() const { return _shouldQuit; };
    void Quit() { _shouldQuit = true; };

private:
    friend Renderer;
    // std::unique_ptr<World> _world;
    // std::unique_ptr<ThreadPool> _threadPool;
    // std::unique_ptr<AssetManager> _AssetManager;

    std::unique_ptr<Editor> _editor;

    std::unique_ptr<Renderer> _renderer;

    std::shared_ptr<SceneDescription> _scene;

    std::shared_ptr<Application> _application;

    glm::ivec2 _lastMousePos {};

    std::chrono::time_point<std::chrono::high_resolution_clock> _lastFrameTime;

    PerformanceTracker _performanceTracker;

    bool _shouldQuit = false;
};
