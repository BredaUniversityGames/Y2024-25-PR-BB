#include "renderer.hpp"

#include <utility>

#include "vulkan_helper.hpp"
#include "imgui_impl_vulkan.h"
#include "model_loader.hpp"
#include "mesh_primitives.hpp"
#include "pipelines/geometry_pipeline.hpp"
#include "pipelines/lighting_pipeline.hpp"
#include "pipelines/skydome_pipeline.hpp"
#include "pipelines/tonemapping_pipeline.hpp"
#include "pipelines/gaussian_blur_pipeline.hpp"
#include "pipelines/ibl_pipeline.hpp"
#include "pipelines/shadow_pipeline.hpp"
#include "particles/particle_pipeline.hpp"
#include "pipelines/debug_pipeline.hpp"
#include "gbuffers.hpp"
#include "application_module.hpp"
#include "old_engine.hpp"
#include "single_time_commands.hpp"
#include "batch_buffer.hpp"
#include "ECS.hpp"
#include "gpu_scene.hpp"
#include "log.hpp"
#include "profile_macros.hpp"

#include "stb/stb_image.h"

Renderer::Renderer(ApplicationModule& application, const std::shared_ptr<ECS>& ecs)
    : _brain(application.GetVulkanInfo())
    , _application(application)
    , _ecs(ecs)
    , _bloomSettings(_brain)
{

    auto vulkanInfo = application.GetVulkanInfo();
    _swapChain = std::make_unique<SwapChain>(_brain, glm::uvec2 { vulkanInfo.width, vulkanInfo.height });

    InitializeHDRTarget();
    InitializeBloomTargets();
    LoadEnvironmentMap();

    _modelLoader = std::make_unique<ModelLoader>(_brain);

    _batchBuffer = std::make_unique<BatchBuffer>(_brain, 256 * 1024 * 1024, 256 * 1024 * 1024);

    SingleTimeCommands commandBufferPrimitive { _brain };
    ResourceHandle<Mesh> uvSphere = _modelLoader->LoadMesh(GenerateUVSphere(32, 32), commandBufferPrimitive, *_batchBuffer, ResourceHandle<Material>::Invalid());
    commandBufferPrimitive.Submit();

    _gBuffers = std::make_unique<GBuffers>(_brain, _swapChain->GetImageSize());
    _iblPipeline = std::make_unique<IBLPipeline>(_brain, _environmentMap);

    SingleTimeCommands commandBufferIBL { _brain };
    _iblPipeline->RecordCommands(commandBufferIBL.CommandBuffer());
    commandBufferIBL.Submit();

    GPUSceneCreation gpuSceneCreation {
        _brain,
        _iblPipeline->IrradianceMap(),
        _iblPipeline->PrefilterMap(),
        _iblPipeline->BRDFLUTMap(),
        _gBuffers->Shadow()
    };

    _gpuScene = std::make_unique<GPUScene>(gpuSceneCreation);

    _camera = std::make_unique<CameraResource>(_brain);

    _geometryPipeline = std::make_unique<GeometryPipeline>(_brain, *_gBuffers, *_camera, *_gpuScene);
    _skydomePipeline = std::make_unique<SkydomePipeline>(_brain, std::move(uvSphere), *_camera, _hdrTarget, _brightnessTarget, _environmentMap, _bloomSettings);
    _tonemappingPipeline = std::make_unique<TonemappingPipeline>(_brain, _hdrTarget, _bloomTarget, *_swapChain, _bloomSettings);
    _bloomBlurPipeline = std::make_unique<GaussianBlurPipeline>(_brain, _brightnessTarget, _bloomTarget);
    _shadowPipeline = std::make_unique<ShadowPipeline>(_brain, *_gBuffers, *_gpuScene);
    _debugPipeline = std::make_unique<DebugPipeline>(_brain, *_gBuffers, *_camera, *_swapChain, *_gpuScene);
    _lightingPipeline = std::make_unique<LightingPipeline>(_brain, *_gBuffers, _hdrTarget, _brightnessTarget, *_gpuScene, *_camera, _bloomSettings);
    _particlePipeline = std::make_unique<ParticlePipeline>(_brain, *_camera);

    CreateCommandBuffers();
    CreateSyncObjects();
}

std::vector<std::shared_ptr<ModelHandle>> Renderer::FrontLoadModels(const std::vector<std::string>& models)
{
    uint32_t totalVertexSize {};
    uint32_t totalIndexSize {};
    for (const auto& path : models)
    {
        uint32_t vertexSize;
        uint32_t indexSize;

        _modelLoader->ReadGeometrySize(path, vertexSize, indexSize);
        totalVertexSize += vertexSize;
        totalIndexSize += indexSize;
    }

    bblog::info("vertex size: {}\nindex size: {}", totalVertexSize, totalIndexSize);

    std::vector<std::shared_ptr<ModelHandle>> loadedModels {};

    for (const auto& path : models)
    {
        loadedModels.emplace_back(std::make_shared<ModelHandle>(_modelLoader->Load(path, *_batchBuffer)));
    }

    return loadedModels;
}

Renderer::~Renderer()
{
    _modelLoader.reset();

    _brain.GetImageResourceManager().Destroy(_environmentMap);
    _brain.GetImageResourceManager().Destroy(_hdrTarget);

    _brain.GetImageResourceManager().Destroy(_brightnessTarget);
    _brain.GetImageResourceManager().Destroy(_bloomTarget);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        _brain.device.destroy(_inFlightFences[i]);
        _brain.device.destroy(_renderFinishedSemaphores[i]);
        _brain.device.destroy(_imageAvailableSemaphores[i]);
    }

    for (auto& model : _scene->models)
    {
        for (auto& texture : model->textures)
        {
            _brain.GetImageResourceManager().Destroy(texture);
        }
        for (auto& material : model->materials)
        {
            _brain.GetMaterialResourceManager().Destroy(material);
        }
    }

    _swapChain.reset();
}

void Renderer::CreateCommandBuffers()
{
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo {};
    commandBufferAllocateInfo.commandPool = _brain.commandPool;
    commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
    commandBufferAllocateInfo.commandBufferCount = _commandBuffers.size();

    util::VK_ASSERT(_brain.device.allocateCommandBuffers(&commandBufferAllocateInfo, _commandBuffers.data()),
        "Failed allocating command buffer!");
}

void Renderer::RecordCommandBuffer(const vk::CommandBuffer& commandBuffer, uint32_t swapChainImageIndex, float deltaTime)
{
    ZoneScoped;

    // Since there is only one scene, we can reuse the same gpu buffers
    _gpuScene->Update(*_scene, _currentFrame);

    const RenderSceneDescription sceneDescription {
        .gpuScene = *_gpuScene,
        .sceneDescription = *_scene,
        .batchBuffer = *_batchBuffer
    };

    _brain.drawStats = {};

    const Image* hdrImage = _brain.GetImageResourceManager().Access(_hdrTarget);
    const Image* hdrBloomImage = _brain.GetImageResourceManager().Access(_brightnessTarget);
    const Image* hdrBlurredBloomImage = _brain.GetImageResourceManager().Access(_bloomTarget);
    const Image* shadowMap = _brain.GetImageResourceManager().Access(_gBuffers->Shadow());

    vk::CommandBufferBeginInfo commandBufferBeginInfo {};
    util::VK_ASSERT(commandBuffer.begin(&commandBufferBeginInfo), "Failed to begin recording command buffer!");

    util::TransitionImageLayout(commandBuffer, _swapChain->GetImage(swapChainImageIndex), _swapChain->GetFormat(),
        vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
    util::TransitionImageLayout(commandBuffer, hdrImage->image, hdrImage->format, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal);
    _gBuffers->TransitionLayout(commandBuffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

    util::TransitionImageLayout(commandBuffer, shadowMap->image, shadowMap->format, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal, 1, 0, 1, vk::ImageAspectFlagBits::eDepth);
    _geometryPipeline->RecordCommands(commandBuffer, _currentFrame, sceneDescription);
    _shadowPipeline->RecordCommands(commandBuffer, _currentFrame, sceneDescription);

    _gBuffers->TransitionLayout(commandBuffer, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    util::TransitionImageLayout(commandBuffer, hdrBloomImage->image, hdrBloomImage->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
    util::TransitionImageLayout(commandBuffer, shadowMap->image, shadowMap->format, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 1, 0, 1, vk::ImageAspectFlagBits::eDepth);

    _skydomePipeline->RecordCommands(commandBuffer, _currentFrame, sceneDescription);
    _lightingPipeline->RecordCommands(commandBuffer, _currentFrame, sceneDescription);

    _particlePipeline->RecordCommands(commandBuffer, *_ecs, deltaTime);

    util::TransitionImageLayout(commandBuffer, shadowMap->image, shadowMap->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilReadOnlyOptimal, 1, 0, 1, vk::ImageAspectFlagBits::eDepth);
    util::TransitionImageLayout(commandBuffer, hdrBloomImage->image, hdrBloomImage->format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    util::TransitionImageLayout(commandBuffer, hdrBlurredBloomImage->image, hdrBlurredBloomImage->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

    _bloomBlurPipeline->RecordCommands(commandBuffer, _currentFrame, 5);

    util::TransitionImageLayout(commandBuffer, hdrImage->image, hdrImage->format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    util::TransitionImageLayout(commandBuffer, hdrBlurredBloomImage->image, hdrBlurredBloomImage->format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    _tonemappingPipeline->RecordCommands(commandBuffer, _currentFrame, swapChainImageIndex);

    _debugPipeline->RecordCommands(commandBuffer, _currentFrame, swapChainImageIndex);

    util::TransitionImageLayout(commandBuffer, _swapChain->GetImage(swapChainImageIndex), _swapChain->GetFormat(),
        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);
    commandBuffer.end();
}

void Renderer::CreateSyncObjects()
{
    vk::SemaphoreCreateInfo semaphoreCreateInfo {};
    vk::FenceCreateInfo fenceCreateInfo {};
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    std::string errorMsg { "Failed creating sync object!" };
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        util::VK_ASSERT(_brain.device.createSemaphore(&semaphoreCreateInfo, nullptr, &_imageAvailableSemaphores[i]), errorMsg);
        util::VK_ASSERT(_brain.device.createSemaphore(&semaphoreCreateInfo, nullptr, &_renderFinishedSemaphores[i]), errorMsg);
        util::VK_ASSERT(_brain.device.createFence(&fenceCreateInfo, nullptr, &_inFlightFences[i]), errorMsg);
    }
}

void Renderer::InitializeHDRTarget()
{
    auto size = _swapChain->GetImageSize();

    ImageCreation hdrCreation {};
    hdrCreation.SetName("HDR Target").SetSize(size.x, size.y).SetFormat(vk::Format::eR32G32B32A32Sfloat).SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    _hdrTarget = _brain.GetImageResourceManager().Create(hdrCreation);
}

void Renderer::InitializeBloomTargets()
{
    auto size = _swapChain->GetImageSize();

    ImageCreation hdrBloomCreation {};
    hdrBloomCreation.SetName("HDR Bloom Target").SetSize(size.x, size.y).SetFormat(vk::Format::eR16G16B16A16Sfloat).SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    ImageCreation hdrBlurredBloomCreation {};
    hdrBlurredBloomCreation.SetName("HDR Blurred Bloom Target").SetSize(size.x, size.y).SetFormat(vk::Format::eR16G16B16A16Sfloat).SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    _brightnessTarget = _brain.GetImageResourceManager().Create(hdrBloomCreation);
    _bloomTarget = _brain.GetImageResourceManager().Create(hdrBlurredBloomCreation);
}

void Renderer::LoadEnvironmentMap()
{
    int32_t width, height, numChannels;
    float* stbiData = stbi_loadf("assets/hdri/industrial_sunset_02_puresky_4k.hdr", &width, &height, &numChannels, 4);

    if (stbiData == nullptr)
        throw std::runtime_error("Failed loading HDRI!");

    std::vector<std::byte> data(width * height * 4 * sizeof(float));
    std::memcpy(data.data(), stbiData, data.size());

    stbi_image_free(stbiData);

    ImageCreation envMapCreation {};
    envMapCreation.SetSize(width, height).SetFlags(vk::ImageUsageFlagBits::eSampled).SetName("Environment HDRI").SetData(data.data()).SetFormat(vk::Format::eR32G32B32A32Sfloat);
    envMapCreation.isHDR = true;

    _environmentMap = _brain.GetImageResourceManager().Create(envMapCreation);
}

void Renderer::UpdateBindless()
{
    _brain.UpdateBindlessSet();
}
void Renderer::Render(float deltaTime)
{
    ZoneNamedN(zz, "Renderer::Render()", true);

    {
        ZoneNamedN(zz, "Wait On Fence", true);
        util::VK_ASSERT(_brain.device.waitForFences(1, &_inFlightFences[_currentFrame], vk::True, std::numeric_limits<uint64_t>::max()),
            "Failed waiting on in flight fence!");
    }

    _bloomSettings.Update(_currentFrame);

    // TODO: handle this more gracefully
    assert(_scene->camera.aspectRatio > 0.0f && "Camera with invalid aspect ratio");
    _camera->Update(_currentFrame, _scene->camera);

    uint32_t imageIndex {};
    vk::Result result {};

    {
        ZoneNamedN(zz, "Acquire Next Image", true);

        result = _brain.device.acquireNextImageKHR(_swapChain->GetSwapChain(), std::numeric_limits<uint64_t>::max(),
            _imageAvailableSemaphores[_currentFrame], nullptr, &imageIndex);

        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
        {
            _swapChain->Resize(_application.DisplaySize());
            _gBuffers->Resize(_application.DisplaySize());

            return;
        }
        else
            util::VK_ASSERT(result, "Failed acquiring next image from swap chain!");
    }

    util::VK_ASSERT(_brain.device.resetFences(1, &_inFlightFences[_currentFrame]), "Failed resetting fences!");

    {
        ZoneNamedN(zz, "ImGui Render", true);
        ImGui::Render();
    }

    _commandBuffers[_currentFrame].reset();

    RecordCommandBuffer(_commandBuffers[_currentFrame], imageIndex, deltaTime);

    vk::SubmitInfo submitInfo {};
    vk::Semaphore waitSemaphores[] = { _imageAvailableSemaphores[_currentFrame] };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffers[_currentFrame];

    vk::Semaphore signalSemaphores[] = { _renderFinishedSemaphores[_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    {
        ZoneNamedN(zz, "Submit Commands", true);
        util::VK_ASSERT(_brain.graphicsQueue.submit(1, &submitInfo, _inFlightFences[_currentFrame]), "Failed submitting to graphics queue!");
    }

    vk::PresentInfoKHR presentInfo {};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    vk::SwapchainKHR swapchains[] = { _swapChain->GetSwapChain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    {
        ZoneNamedN(zz, "Present Image", true);
        result = _brain.presentQueue.presentKHR(&presentInfo);
    }

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || _swapChain->GetImageSize() != _application.DisplaySize())
    {
        _swapChain->Resize(_application.DisplaySize());
        _gBuffers->Resize(_application.DisplaySize());
    }
    else
    {
        util::VK_ASSERT(result, "Failed acquiring next image from swap chain!");
    }

    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
