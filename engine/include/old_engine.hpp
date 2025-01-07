#pragma once

#include "module_interface.hpp"

#include <glm/glm.hpp>
#include <memory>

struct RigidbodyComponent;
class ECSModule;
class Renderer;
class Editor;

class OldEngine : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Tick(Engine& engine) override;
    void Shutdown(Engine& engine) override;

    std::string_view GetName() override { return "OldEngine Module"; }

public:
    OldEngine();
    ~OldEngine() override;

    NON_COPYABLE(OldEngine);
    NON_MOVABLE(OldEngine);

    ECSModule& GetECS() const { return *_ecs; }

private:
    // std::unique_ptr<ThreadPool> _threadPool;
    // std::unique_ptr<AssetManager> _AssetManager;

    ECSModule* _ecs;
    glm::ivec2 _lastMousePos {};
    MAYBE_UNUSED bool _shouldQuit = false;
};
