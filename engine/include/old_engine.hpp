#pragma once

#include "module_interface.hpp"

#include <memory>

struct RigidbodyComponent;
class ECSModule;
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

    ECSModule& GetECS() const { return *_ecs; }

private:
    // std::unique_ptr<ThreadPool> _threadPool;
    // std::unique_ptr<AssetManager> _AssetManager;

    ECSModule* _ecs;
    glm::ivec2 _lastMousePos {};
    std::chrono::time_point<std::chrono::high_resolution_clock> _lastFrameTime;

    MAYBE_UNUSED bool _shouldQuit = false;
};
