#pragma once

#include "model.hpp"
#include "module_interface.hpp"

class Renderer;
class ParticleInterface;

struct SceneDescription;

class RendererModule final : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) final;
    void Shutdown(Engine& engine) final;
    void Tick(Engine& engine) final;

public:
    RendererModule();
    ~RendererModule() final = default;

    void SetScene(std::shared_ptr<const SceneDescription> scene);
    std::vector<Model> FrontLoadModels(const std::vector<std::string>& modelPaths);
    Renderer& GetRenderer() { return *_renderer; }
    ParticleInterface& GetParticleInterface() { return *_particleInterface; }

private:
    std::unique_ptr<Renderer> _renderer;
    std::unique_ptr<ParticleInterface> _particleInterface;
};
