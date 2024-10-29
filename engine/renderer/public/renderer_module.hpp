#pragma once

#include "module_interface.hpp"
#include "imgui_impl_vulkan.h"
#include "vulkan/vulkan.hpp"
#include "renderer_public.hpp"

class Renderer;
class ParticleInterface;

class RendererModule final : public ModuleInterface
{
public:
    RendererModule();
    ~RendererModule() final;

    void GetImguiInitInfo(ImGui_ImplVulkan_InitInfo& initInfo, vk::PipelineRenderingCreateInfoKHR& pipelineCreateInfo, vk::Format& swapchainFormat) const;

    void SetScene(std::shared_ptr<SceneDescription> scene);
    std::vector<std::shared_ptr<ModelHandle>> FrontLoadModels(const std::vector<std::string>& models);

private:
    ModuleTickOrder Init(Engine& engine) final;
    void Shutdown(Engine& engine) final;
    void Tick(Engine& engine) final;

    std::unique_ptr<Renderer> _renderer;
    std::unique_ptr<ParticleInterface> _particleInterface;
};