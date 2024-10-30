#include "renderer_module.hpp"

#include <memory>
#include "renderer.hpp"
#include "particles/particle_interface.hpp"
#include "engine.hpp"
#include "application_module.hpp"
#include "old_engine.hpp"
#include "swap_chain.hpp"

RendererModule::RendererModule()
{
}

RendererModule::~RendererModule()
{
}

ModuleTickOrder RendererModule::Init(MAYBE_UNUSED Engine& engine)
{
    _renderer = std::make_unique<Renderer>(engine.GetModule<ApplicationModule>(), engine.GetModule<OldEngine>().GetECS());
    _particleInterface = std::make_unique<ParticleInterface>(engine.GetModule<OldEngine>().GetECS());

    return ModuleTickOrder::eRender;
}

void RendererModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    _renderer->GetBrain().device.waitIdle();
}

void RendererModule::Tick(MAYBE_UNUSED Engine& engine)
{
    _renderer->Render(engine.GetModule<OldEngine>().DeltaTimeMS());
}

void RendererModule::GetImguiInitInfo(ImGui_ImplVulkan_InitInfo& initInfo, vk::PipelineRenderingCreateInfoKHR& pipelineCreateInfo, vk::Format& swapchainFormat) const
{
    pipelineCreateInfo.colorAttachmentCount = 1;
    swapchainFormat = _renderer->GetSwapChain().GetFormat();
    pipelineCreateInfo.pColorAttachmentFormats = &swapchainFormat;
    pipelineCreateInfo.depthAttachmentFormat = _renderer->GetDepthFormat();

    initInfo.UseDynamicRendering = true;
    initInfo.PipelineRenderingCreateInfo = static_cast<VkPipelineRenderingCreateInfo>(pipelineCreateInfo);
    initInfo.PhysicalDevice = _renderer->GetBrain().physicalDevice;
    initInfo.Device = _renderer->GetBrain().device;
    initInfo.ImageCount = MAX_FRAMES_IN_FLIGHT;
    initInfo.Instance = _renderer->GetBrain().instance;
    initInfo.MSAASamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
    initInfo.Queue = _renderer->GetBrain().graphicsQueue;
    initInfo.QueueFamily = _renderer->GetBrain().queueFamilyIndices.graphicsFamily.value();
    initInfo.DescriptorPool = _renderer->GetBrain().descriptorPool;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = _renderer->GetSwapChain().GetImageCount();
}

void RendererModule::SetScene(std::shared_ptr<SceneDescription> scene)
{
    _renderer->SetScene(scene);
}
std::vector<std::shared_ptr<ModelHandle>> RendererModule::FrontLoadModels(const std::vector<std::string>& models)
{
    auto result = _renderer->FrontLoadModels(models);
    _renderer->GetBrain().UpdateBindlessSet();
    return result;
}
