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
#include "frame_graph.hpp"

#include "stb/stb_image.h"

Renderer::Renderer(ApplicationModule& application, const std::shared_ptr<ECS> ecs)
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

    _modelLoader = std::make_unique<ModelLoader>(_brain, _ecs);

    _batchBuffer = std::make_shared<BatchBuffer>(_brain, 256 * 1024 * 1024, 256 * 1024 * 1024);

    SingleTimeCommands commandBufferPrimitive { _brain };
    ResourceHandle<Mesh> uvSphere = _modelLoader->LoadMesh(GenerateUVSphere(32, 32), commandBufferPrimitive, *_batchBuffer, ResourceHandle<Material>::Invalid());
    commandBufferPrimitive.Submit();

    _gBuffers = std::make_unique<GBuffers>(_brain, _swapChain->GetImageSize());
    _iblPipeline = std::make_unique<IBLPipeline>(_brain, _environmentMap);

    // Makes sure previously created textures are available to be sampled in the IBL pipeline
    UpdateBindless();

    SingleTimeCommands commandBufferIBL { _brain };
    _iblPipeline->RecordCommands(commandBufferIBL.CommandBuffer());
    commandBufferIBL.Submit();

    GPUSceneCreation gpuSceneCreation {
        _brain,
        _ecs,
        _iblPipeline->IrradianceMap(),
        _iblPipeline->PrefilterMap(),
        _iblPipeline->BRDFLUTMap(),
        _gBuffers->Shadow()
    };

    _gpuScene = std::make_shared<GPUScene>(gpuSceneCreation);

    _camera = std::make_unique<CameraResource>(_brain);

    _geometryPipeline = std::make_unique<GeometryPipeline>(_brain, *_gBuffers, *_camera, *_gpuScene);
    _skydomePipeline = std::make_unique<SkydomePipeline>(_brain, std::move(uvSphere), *_camera, _hdrTarget, _brightnessTarget, _environmentMap, *_gBuffers, _bloomSettings);
    _tonemappingPipeline = std::make_unique<TonemappingPipeline>(_brain, _hdrTarget, _bloomTarget, *_swapChain, _bloomSettings);
    _bloomBlurPipeline = std::make_unique<GaussianBlurPipeline>(_brain, _brightnessTarget, _bloomTarget);
    _shadowPipeline = std::make_unique<ShadowPipeline>(_brain, *_gBuffers, *_gpuScene);
    _debugPipeline = std::make_unique<DebugPipeline>(_brain, *_gBuffers, *_camera, *_swapChain);
    _lightingPipeline = std::make_unique<LightingPipeline>(_brain, *_gBuffers, _hdrTarget, _brightnessTarget, *_camera, _bloomSettings);
    _particlePipeline = std::make_unique<ParticlePipeline>(_brain, *_camera, *_swapChain);

    CreateCommandBuffers();
    CreateSyncObjects();

    FrameGraphNodeCreation geometryPass { *_geometryPipeline };
    geometryPass.SetName("Geometry pass")
        .SetDebugLabelColor(glm::vec3 { 6.0f, 214.0f, 160.0f } / 255.0f)
        .AddOutput(_gBuffers->Depth(), FrameGraphResourceType::eAttachment)
        .AddOutput(_gBuffers->Attachments()[0], FrameGraphResourceType::eAttachment)
        .AddOutput(_gBuffers->Attachments()[1], FrameGraphResourceType::eAttachment)
        .AddOutput(_gBuffers->Attachments()[2], FrameGraphResourceType::eAttachment)
        .AddOutput(_gBuffers->Attachments()[3], FrameGraphResourceType::eAttachment);

    FrameGraphNodeCreation shadowPass { *_shadowPipeline };
    shadowPass.SetName("Shadow pass")
        .SetDebugLabelColor(glm::vec3 { 0.0f, 1.0f, 1.0f })
        .AddOutput(_gBuffers->Shadow(), FrameGraphResourceType::eAttachment);

    FrameGraphNodeCreation lightingPass { *_lightingPipeline };
    lightingPass.SetName("Lighting pass")
        .SetDebugLabelColor(glm::vec3 { 255.0f, 209.0f, 102.0f } / 255.0f)
        .AddInput(_gBuffers->Attachments()[0], FrameGraphResourceType::eTexture)
        .AddInput(_gBuffers->Attachments()[1], FrameGraphResourceType::eTexture)
        .AddInput(_gBuffers->Attachments()[2], FrameGraphResourceType::eTexture)
        .AddInput(_gBuffers->Attachments()[3], FrameGraphResourceType::eTexture)
        .AddInput(_gBuffers->Shadow(), FrameGraphResourceType::eTexture)
        .AddOutput(_hdrTarget, FrameGraphResourceType::eAttachment)
        .AddOutput(_brightnessTarget, FrameGraphResourceType::eAttachment);

    FrameGraphNodeCreation skyDomePass { *_skydomePipeline };
    skyDomePass.SetName("Sky dome pass")
        .SetDebugLabelColor(glm::vec3 { 17.0f, 138.0f, 178.0f } / 255.0f)
        // Does nothing internally in this situation, for clarity that it is used here
        .AddInput(_gBuffers->Depth(), FrameGraphResourceType::eAttachment)
        // Making sure the sky dome pass runs after the lighting pass with a reference
        .AddInput(_hdrTarget, FrameGraphResourceType::eAttachment | FrameGraphResourceType::eReference)
        // Not needed references, just for clarity this pass also contributes to those targets
        .AddOutput(_hdrTarget, FrameGraphResourceType::eAttachment | FrameGraphResourceType::eReference)
        .AddOutput(_brightnessTarget, FrameGraphResourceType::eAttachment | FrameGraphResourceType::eReference);

    FrameGraphNodeCreation bloomBlurPass { *_bloomBlurPipeline };
    bloomBlurPass.SetName("Bloom gaussian blur pass")
        .SetDebugLabelColor(glm::vec3 { 255.0f, 255.0f, 153.0f } / 255.0f)
        .AddInput(_brightnessTarget, FrameGraphResourceType::eTexture)
        .AddOutput(_bloomTarget, FrameGraphResourceType::eAttachment);

    FrameGraphNodeCreation toneMappingPass { *_tonemappingPipeline };
    toneMappingPass.SetName("Tonemapping pass")
        .SetDebugLabelColor(glm::vec3 { 239.0f, 71.0f, 111.0f } / 255.0f)
        .AddInput(_hdrTarget, FrameGraphResourceType::eTexture)
        .AddInput(_bloomTarget, FrameGraphResourceType::eTexture);

    FrameGraphNodeCreation debugPass { *_debugPipeline };
    debugPass.SetName("Debug pass")
        .SetDebugLabelColor(glm::vec3 { 0.0f, 1.0f, 1.0f })
        // Does nothing internally in this situation, used for clarity that the debug pass uses the depth buffer
        .AddInput(_gBuffers->Depth(), FrameGraphResourceType::eAttachment)
        // Reference to make sure it runs at the end
        .AddInput(_bloomTarget, FrameGraphResourceType::eTexture | FrameGraphResourceType::eReference);

    _frameGraph = std::make_unique<FrameGraph>(_brain, *_swapChain);
    FrameGraph& frameGraph = *_frameGraph;
    frameGraph.AddNode(geometryPass)
        .AddNode(shadowPass)
        .AddNode(skyDomePass)
        .AddNode(lightingPass)
        .AddNode(bloomBlurPass)
        .AddNode(toneMappingPass)
        .AddNode(debugPass)
        .Build();
}

std::vector<Model> Renderer::FrontLoadModels(const std::vector<std::string>& modelPaths)
{
    // TODO: Use this later to determine batch buffer size.
    // uint32_t totalVertexSize {};
    // uint32_t totalIndexSize {};
    // for (const auto& path : models)
    // {
    //     uint32_t vertexSize;
    //     uint32_t indexSize;

    //     _modelLoader->ReadGeometrySize(path, vertexSize, indexSize);
    //     totalVertexSize += vertexSize;
    //     totalIndexSize += indexSize;
    //}

    std::vector<Model> models {};

    for (const auto& path : modelPaths)
    {
        models.emplace_back(_modelLoader->Load(path, *_batchBuffer, ModelLoader::LoadMode::eHierarchical));

        _modelResources.emplace_back(std::make_shared<ModelResources>(models.back().resources));
    }

    return models;
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

    for (auto& model : _modelResources)
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
        .gpuScene = _gpuScene,
        .sceneDescription = _scene,
        .ecs = _ecs,
        .batchBuffer = _batchBuffer,
        .targetSwapChainImageIndex = swapChainImageIndex
    };

    _brain.drawStats = {};

    vk::CommandBufferBeginInfo commandBufferBeginInfo {};
    util::VK_ASSERT(commandBuffer.begin(&commandBufferBeginInfo), "Failed to begin recording command buffer!");

    // Presenting pass currently not supported by frame graph, so this has to be done manually
    util::TransitionImageLayout(commandBuffer, _swapChain->GetImage(swapChainImageIndex), _swapChain->GetFormat(),
        vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

    _particlePipeline->RecordCommands(commandBuffer, _currentFrame, *_ecs, deltaTime); // TODO: Add to frame graph after ECS is integrated into renderer

    _frameGraph->RecordCommands(commandBuffer, _currentFrame, sceneDescription);

    // Presenting pass currently not supported by frame graph, so this has to be done manually
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
