#pragma once

#include "model.hpp"
#include "module_interface.hpp"

class Renderer;

struct SceneDescription;

class RendererModule final : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) final;
    void Shutdown(Engine& engine) final;
    void Tick(Engine& engine) final;

public:
    RendererModule();
    ~RendererModule() final = default;

    std::vector<std::pair<CPUModel, ResourceHandle<GPUModel>>> FrontLoadModels(const std::vector<std::string>& modelPaths);
    std::shared_ptr<Renderer> GetRenderer() { return _renderer; }

private:
    std::shared_ptr<GraphicsContext> _context;
    std::shared_ptr<Renderer> _renderer;
};
