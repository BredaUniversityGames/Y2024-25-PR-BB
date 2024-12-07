#pragma once

#include "model.hpp"
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

public:
    RendererModule();
    ~RendererModule() final = default;

    std::vector<std::pair<CPUModel, ResourceHandle<GPUModel>>> FrontLoadModels(const std::vector<std::string>& modelPaths);
    std::shared_ptr<Renderer> GetRenderer() { return _renderer; }
    std::shared_ptr<ImGuiBackend> GetImGuiBackend() { return _imguiBackend; }
    std::shared_ptr<GraphicsContext> GetGraphicsContext() { return _context; }

private:
    std::shared_ptr<GraphicsContext> _context;
    std::shared_ptr<Renderer> _renderer;
    std::shared_ptr<ImGuiBackend> _imguiBackend;
};
