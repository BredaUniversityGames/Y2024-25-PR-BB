#pragma once

#include "module_interface.hpp"
#include "renderer.hpp"

class PathfindingModule : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) final;
    void Shutdown(Engine& engine) final;
    void Tick(Engine& engine) final;

public:
    PathfindingModule();
    ~PathfindingModule();

    NON_COPYABLE(PathfindingModule)
    NON_MOVABLE(PathfindingModule)

    int SetNavigationMesh(const std::string& mesh);

private:
    std::shared_ptr<Renderer> _renderer;
};