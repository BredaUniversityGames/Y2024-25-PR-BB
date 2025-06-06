#pragma once

#include "cpu_resources.hpp"
#include "module_interface.hpp"

class Renderer;
class ImGuiBackend;
class GraphicsContext;

struct SceneDescription;

class RendererModule final : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) final;
    void Shutdown(Engine& engine) final;
    void Tick(Engine& engine) final;
    std::string_view GetName() override { return "Renderer Module"; }

public:
    RendererModule();
    ~RendererModule() override = default;

    NON_COPYABLE(RendererModule);
    NON_MOVABLE(RendererModule);

    std::vector<ResourceHandle<GPUModel>> LoadModels(const std::vector<CPUModel>& cpuModels);

    std::shared_ptr<Renderer> GetRenderer() { return _renderer; }
    std::shared_ptr<GraphicsContext> GetGraphicsContext() { return _context; }

private:
    std::shared_ptr<GraphicsContext> _context;
    std::shared_ptr<Renderer> _renderer;
};
