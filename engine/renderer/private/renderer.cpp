#include "renderer.hpp"

#include <imgui.h>
#include <memory>
#include <stb_image.h>
#include <utility>

#include "application_module.hpp"
#include "batch_buffer.hpp"
#include "build_hzb_pipeline.hpp"
#include "camera_batch.hpp"
#include "colors.hpp"
#include "ecs_module.hpp"
#include "fonts.hpp"
#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "mesh_primitives.hpp"
#include "model_loader.hpp"
#include "pipelines/debug_pipeline.hpp"
#include "pipelines/fxaa_pipeline.hpp"
#include "pipelines/gaussian_blur_pipeline.hpp"
#include "pipelines/geometry_pipeline.hpp"
#include "pipelines/ibl_pipeline.hpp"
#include "pipelines/lighting_pipeline.hpp"
#include "pipelines/particle_pipeline.hpp"
#include "pipelines/presentation_pipeline.hpp"
#include "pipelines/shadow_pipeline.hpp"
#include "pipelines/skydome_pipeline.hpp"
#include "pipelines/ssao_pipeline.hpp"
#include "pipelines/tonemapping_pipeline.hpp"
#include "pipelines/ui_pipeline.hpp"
#include "profile_macros.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/mesh_resource_manager.hpp"
#include "resource_management/model_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"
#include "single_time_commands.hpp"
#include "ui_main_menu.hpp"
#include "ui_module.hpp"
#include "viewport.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

Renderer::Renderer(ApplicationModule& application, Viewport& viewport, const std::shared_ptr<GraphicsContext>& context, ECSModule& ecs)
    : _context(context)
    , _application(application)
    , _viewport(viewport)
    , _ecs(ecs)
{
    _bloomSettings = std::make_unique<BloomSettings>(_context);

    auto vulkanInfo = application.GetVulkanInfo();
    _swapChain = std::make_unique<SwapChain>(_context, glm::uvec2 { vulkanInfo.width, vulkanInfo.height });

    InitializeHDRTarget();
    InitializeBloomTargets();
    InitializeSSAOTarget();
    InitializeTonemappingTarget();
    InitializeFXAATarget();
    LoadEnvironmentMap();

    _modelLoader = std::make_unique<ModelLoader>();

    const uint32_t mb128 = 128 * 1024 * 1024;
    _staticBatchBuffer = std::make_shared<BatchBuffer>(_context, mb128, mb128);
    _skinnedBatchBuffer = std::make_shared<BatchBuffer>(_context, mb128, mb128);

    SingleTimeCommands commandBufferPrimitive { _context->VulkanContext() };
    ResourceHandle<GPUMesh> uvSphere = _context->Resources()->MeshResourceManager().Create(GenerateUVSphere(32, 32), ResourceHandle<GPUMaterial>::Null(), *_staticBatchBuffer);
    commandBufferPrimitive.Submit();

    _gBuffers = std::make_unique<GBuffers>(_context, _swapChain->GetImageSize());
    _iblPipeline = std::make_unique<IBLPipeline>(_context, _environmentMap);

    // Makes sure previously created textures are available to be sampled in the IBL pipeline
    UpdateBindless();

    SingleTimeCommands commandBufferIBL { _context->VulkanContext() };
    _iblPipeline->RecordCommands(commandBufferIBL.CommandBuffer());
    commandBufferIBL.Submit();

    GPUSceneCreation gpuSceneCreation {
        _context,
        _ecs,
        _iblPipeline->IrradianceMap(),
        _iblPipeline->PrefilterMap(),
        _iblPipeline->BRDFLUTMap(),
        _gBuffers->Depth(),
        _application.DisplaySize(),
    };

    _gpuScene = std::make_shared<GPUScene>(gpuSceneCreation);

    _generateMainDrawsPipeline = std::make_unique<GenerateDrawsPipeline>(_context, _gpuScene->MainCameraBatch());
    _generateShadowDrawsPipeline = std::make_unique<GenerateDrawsPipeline>(_context, _gpuScene->ShadowCameraBatch());
    _buildMainHzbPipeline = std::make_unique<BuildHzbPipeline>(_context, _gpuScene->MainCameraBatch(), true);
    _buildShadowHzbPipeline = std::make_unique<BuildHzbPipeline>(_context, _gpuScene->ShadowCameraBatch(), false);
    _geometryPipeline = std::make_unique<GeometryPipeline>(_context, *_gBuffers, _gpuScene->MainCameraBatch());
    _shadowPipeline = std::make_unique<ShadowPipeline>(_context, *_gpuScene, _gpuScene->ShadowCameraBatch());
    _skydomePipeline = std::make_unique<SkydomePipeline>(_context, uvSphere, _hdrTarget, _brightnessTarget, _environmentMap, *_gBuffers, *_bloomSettings);
    _tonemappingPipeline = std::make_unique<TonemappingPipeline>(_context, _hdrTarget, _bloomTarget, _tonemappingTarget, *_swapChain, *_bloomSettings);
    _fxaaPipeline = std::make_unique<FXAAPipeline>(_context, *_gBuffers, _fxaaTarget, _tonemappingTarget);
    _uiPipeline = std::make_unique<UIPipeline>(_context, _fxaaTarget, *_swapChain);
    _bloomBlurPipeline = std::make_unique<GaussianBlurPipeline>(_context, _brightnessTarget, _bloomTarget);
    _ssaoPipeline = std::make_unique<SSAOPipeline>(_context, *_gBuffers, _ssaoTarget);
    _debugPipeline = std::make_unique<DebugPipeline>(_context, *_swapChain, *_gBuffers, _fxaaTarget);
    _lightingPipeline = std::make_unique<LightingPipeline>(_context, *_gBuffers, _hdrTarget, _brightnessTarget, *_bloomSettings, _ssaoTarget);
    _particlePipeline = std::make_unique<ParticlePipeline>(_context, _ecs, *_gBuffers, _hdrTarget, _brightnessTarget, *_bloomSettings);
    _presentationPipeline = std::make_unique<PresentationPipeline>(_context, *_swapChain, _fxaaTarget);

    CreateCommandBuffers();
    CreateSyncObjects();

    FrameGraphNodeCreation generateMainDrawsPrepass { *_generateMainDrawsPipeline, FrameGraphRenderPassType::eCompute };
    generateMainDrawsPrepass.SetName("Generate main draws prepass")
        .SetDebugLabelColor(GetColor(ColorType::Crimson))
        .AddOutput(_gpuScene->MainCameraBatch().DrawBuffer(), FrameGraphResourceType::eBuffer, vk::PipelineStageFlagBits2::eComputeShader);

    FrameGraphNodeCreation generateMainDrawsSecondPass { *_generateMainDrawsPipeline, FrameGraphRenderPassType::eCompute };
    generateMainDrawsSecondPass.SetName("Generate main draws second pass")
        .SetDebugLabelColor(GetColor(ColorType::Crimson))
        .AddInput(_gpuScene->MainCameraBatch().HZBImage(), FrameGraphResourceType::eTexture)
        .AddOutput(_gpuScene->MainCameraBatch().DrawBuffer(), FrameGraphResourceType::eBuffer, vk::PipelineStageFlagBits2::eComputeShader);

    FrameGraphNodeCreation buildMainHZBPass { *_buildMainHzbPipeline, FrameGraphRenderPassType::eCompute };
    buildMainHZBPass.SetName("Build main HZB pass")
        .SetDebugLabelColor(GetColor(ColorType::Emerald))
        .AddInput(_gpuScene->MainCameraBatch().DepthImage(), FrameGraphResourceType::eTexture)
        .AddOutput(_gpuScene->MainCameraBatch().HZBImage(), FrameGraphResourceType::eTexture);

    FrameGraphNodeCreation generateShadowDrawsPrepass { *_generateShadowDrawsPipeline, FrameGraphRenderPassType::eCompute };
    generateShadowDrawsPrepass.SetName("Generate shadow draws prepass")
        .SetDebugLabelColor(GetColor(ColorType::Goldenrod))
        .AddOutput(_gpuScene->ShadowCameraBatch().DrawBuffer(), FrameGraphResourceType::eBuffer, vk::PipelineStageFlagBits2::eComputeShader);

    FrameGraphNodeCreation generateShadowDrawsSecondPass { *_generateShadowDrawsPipeline, FrameGraphRenderPassType::eCompute };
    generateShadowDrawsSecondPass.SetName("Generate shadow draws second pass")
        .SetDebugLabelColor(GetColor(ColorType::Goldenrod))
        .AddInput(_gpuScene->ShadowCameraBatch().HZBImage(), FrameGraphResourceType::eTexture)
        .AddOutput(_gpuScene->ShadowCameraBatch().DrawBuffer(), FrameGraphResourceType::eBuffer, vk::PipelineStageFlagBits2::eComputeShader);

    FrameGraphNodeCreation buildShadowHZBPass { *_buildShadowHzbPipeline, FrameGraphRenderPassType::eCompute };
    buildShadowHZBPass.SetName("Build shadow HZB pass")
        .SetDebugLabelColor(GetColor(ColorType::Emerald))
        .AddInput(_gpuScene->ShadowCameraBatch().DepthImage(), FrameGraphResourceType::eTexture)
        .AddOutput(_gpuScene->ShadowCameraBatch().HZBImage(), FrameGraphResourceType::eTexture);

    FrameGraphNodeCreation geometryPrepass { *_geometryPipeline };
    geometryPrepass.SetName("Geometry prepass")
        .SetDebugLabelColor(GetColor(ColorType::Cyan))
        .AddInput(_gpuScene->MainCameraBatch().DrawBuffer(), FrameGraphResourceType::eBuffer, vk::PipelineStageFlagBits2::eDrawIndirect)
        .AddOutput(_gBuffers->Depth(), FrameGraphResourceType::eAttachment)
        .AddOutput(_gBuffers->Attachments()[0], FrameGraphResourceType::eAttachment)
        .AddOutput(_gBuffers->Attachments()[1], FrameGraphResourceType::eAttachment)
        .AddOutput(_gBuffers->Attachments()[2], FrameGraphResourceType::eAttachment)
        .AddOutput(_gBuffers->Attachments()[3], FrameGraphResourceType::eAttachment);

    FrameGraphNodeCreation geometrySecondPass { *_geometryPipeline };
    geometrySecondPass.SetName("Geometry second pass")
        .SetDebugLabelColor(GetColor(ColorType::Cyan))
        .AddInput(_gpuScene->MainCameraBatch().DrawBuffer(), FrameGraphResourceType::eBuffer, vk::PipelineStageFlagBits2::eDrawIndirect)
        .AddOutput(_gBuffers->Depth(), FrameGraphResourceType::eAttachment)
        .AddOutput(_gBuffers->Attachments()[0], FrameGraphResourceType::eAttachment)
        .AddOutput(_gBuffers->Attachments()[1], FrameGraphResourceType::eAttachment)
        .AddOutput(_gBuffers->Attachments()[2], FrameGraphResourceType::eAttachment)
        .AddOutput(_gBuffers->Attachments()[3], FrameGraphResourceType::eAttachment);

    FrameGraphNodeCreation shadowPrepass { *_shadowPipeline };
    shadowPrepass.SetName("Shadow prepass")
        .SetDebugLabelColor(GetColor(ColorType::Orange))
        .AddInput(_gpuScene->ShadowCameraBatch().DrawBuffer(), FrameGraphResourceType::eBuffer, vk::PipelineStageFlagBits2::eDrawIndirect)
        .AddOutput(_gpuScene->Shadow(), FrameGraphResourceType::eAttachment);

    FrameGraphNodeCreation shadowSecondPass { *_shadowPipeline };
    shadowSecondPass.SetName("Shadow second pass")
        .SetDebugLabelColor(GetColor(ColorType::Orange))
        .AddInput(_gpuScene->ShadowCameraBatch().DrawBuffer(), FrameGraphResourceType::eBuffer, vk::PipelineStageFlagBits2::eDrawIndirect)
        .AddOutput(_gpuScene->Shadow(), FrameGraphResourceType::eAttachment);

    FrameGraphNodeCreation ssaoPass { *_ssaoPipeline };
    ssaoPass.SetName("SSAO pass")
        .SetDebugLabelColor(GetColor(ColorType::Mint))
        .AddInput(_gBuffers->Attachments()[1], FrameGraphResourceType::eTexture)
        .AddInput(_gBuffers->Attachments()[3], FrameGraphResourceType::eTexture)
        .AddOutput(_ssaoTarget, FrameGraphResourceType::eAttachment);

    FrameGraphNodeCreation lightingPass { *_lightingPipeline };
    lightingPass.SetName("Lighting pass")
        .SetDebugLabelColor(GetColor(ColorType::Periwinkle))
        .AddInput(_gBuffers->Attachments()[0], FrameGraphResourceType::eTexture)
        .AddInput(_gBuffers->Attachments()[1], FrameGraphResourceType::eTexture)
        .AddInput(_gBuffers->Attachments()[2], FrameGraphResourceType::eTexture)
        .AddInput(_gBuffers->Attachments()[3], FrameGraphResourceType::eTexture)
        .AddInput(_ssaoTarget, FrameGraphResourceType::eTexture)
        .AddInput(_gpuScene->Shadow(), FrameGraphResourceType::eTexture)
        .AddOutput(_hdrTarget, FrameGraphResourceType::eAttachment)
        .AddOutput(_brightnessTarget, FrameGraphResourceType::eAttachment);

    FrameGraphNodeCreation skyDomePass { *_skydomePipeline };
    skyDomePass.SetName("Sky dome pass")
        .SetDebugLabelColor(GetColor(ColorType::Pistachio))
        .AddInput(_gBuffers->Depth(), FrameGraphResourceType::eAttachment)
        .AddOutput(_hdrTarget, FrameGraphResourceType::eAttachment, true)
        .AddOutput(_brightnessTarget, FrameGraphResourceType::eAttachment, true);

    FrameGraphNodeCreation particlePass { *_particlePipeline };
    particlePass.SetName("Particle pass")
        .SetDebugLabelColor(GetColor(ColorType::Plum))
        .AddInput(_gBuffers->Depth(), FrameGraphResourceType::eAttachment)
        .AddOutput(_hdrTarget, FrameGraphResourceType::eAttachment)
        .AddOutput(_brightnessTarget, FrameGraphResourceType::eAttachment);

    FrameGraphNodeCreation bloomBlurPass { *_bloomBlurPipeline };
    bloomBlurPass.SetName("Bloom gaussian blur pass")
        .SetDebugLabelColor(GetColor(ColorType::Rose))
        .AddInput(_brightnessTarget, FrameGraphResourceType::eTexture)
        .AddOutput(_bloomTarget, FrameGraphResourceType::eAttachment);

    FrameGraphNodeCreation toneMappingPass { *_tonemappingPipeline };
    toneMappingPass.SetName("Tonemapping pass")
        .SetDebugLabelColor(GetColor(ColorType::Seafoam))
        .AddInput(_hdrTarget, FrameGraphResourceType::eTexture)
        .AddInput(_bloomTarget, FrameGraphResourceType::eTexture)
        .AddOutput(_tonemappingTarget, FrameGraphResourceType::eAttachment);

    FrameGraphNodeCreation fxaaPass { *_fxaaPipeline };
    fxaaPass.SetName("FXAA pass")
        .SetDebugLabelColor(GetColor(ColorType::Sand))
        .AddInput(_tonemappingTarget, FrameGraphResourceType::eTexture)
        .AddOutput(_fxaaTarget, FrameGraphResourceType::eAttachment);

    // TODO: THIS PASS SHOULD BE DONE LAST.
    FrameGraphNodeCreation uiPass { *_uiPipeline };
    uiPass.SetName("UI pass")
        .SetDebugLabelColor(GetColor(ColorType::Teal))
        .AddOutput(_fxaaTarget, FrameGraphResourceType::eAttachment);

    FrameGraphNodeCreation debugPass { *_debugPipeline };
    debugPass.SetName("Debug pass")
        .SetDebugLabelColor(GetColor(ColorType::BrightTeal))
        .AddInput(_gBuffers->Depth(), FrameGraphResourceType::eAttachment)
        .AddOutput(_fxaaTarget, FrameGraphResourceType::eAttachment);

    FrameGraphNodeCreation presentationPass { *_presentationPipeline };
    presentationPass.SetName("Presentation pass")
        .SetDebugLabelColor(GetColor(ColorType::SkyBlue))
        // No support for presentation targets in frame graph, so we'll have to this for now
        .AddInput(_fxaaTarget, FrameGraphResourceType::eTexture | FrameGraphResourceType::eReference);

    _frameGraph = std::make_unique<FrameGraph>(_context, *_swapChain);
    FrameGraph& frameGraph = *_frameGraph;
    frameGraph
        .AddNode(generateMainDrawsPrepass)
        .AddNode(generateShadowDrawsPrepass)
        .AddNode(geometryPrepass)
        .AddNode(shadowPrepass)
        .AddNode(buildMainHZBPass)
        .AddNode(buildShadowHZBPass)
        .AddNode(generateMainDrawsSecondPass)
        .AddNode(generateShadowDrawsSecondPass)
        .AddNode(geometrySecondPass)
        .AddNode(shadowSecondPass)
        .AddNode(ssaoPass)
        .AddNode(lightingPass)
        .AddNode(skyDomePass)
        .AddNode(particlePass)
        .AddNode(bloomBlurPass)
        .AddNode(toneMappingPass)
        .AddNode(fxaaPass)
        .AddNode(uiPass)
        .AddNode(debugPass)
        .AddNode(presentationPass)
        .Build();

    static std::array<std::string, MAX_FRAMES_IN_FLIGHT> contextNames { "Command Buffer 0", "Command Buffer 1", "Command Buffer 2" };

    for (size_t i = 0; i < _tracyContexts.size(); ++i)
    {
        _tracyContexts[i] = TracyVkContextCalibrated(
            _context->VulkanContext()->PhysicalDevice(),
            _context->VulkanContext()->Device(),
            _context->VulkanContext()->GraphicsQueue(),
            _commandBuffers[i],
            reinterpret_cast<PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT>(_context->VulkanContext()->Instance().getProcAddr("vkGetPhysicalDeviceCalibrateableTimeDomainsEXT")),
            reinterpret_cast<PFN_vkGetCalibratedTimestampsEXT>(_context->VulkanContext()->Instance().getProcAddr("vkGetCalibratedTimestampsEXT")));
        TracyVkContextName(_tracyContexts[i], contextNames[i].c_str(), contextNames[i].size());
    }
}

std::vector<std::pair<CPUModel, ResourceHandle<GPUModel>>> Renderer::FrontLoadModels(const std::vector<std::string>& modelPaths)
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

    std::vector<std::pair<CPUModel, ResourceHandle<GPUModel>>> models;
    SingleTimeCommands commands { _context->VulkanContext() };
    for (const auto& path : modelPaths)
    {
        auto cpu = _modelLoader->ExtractModelFromGltfFile(path);
        auto gpu = _context->Resources()->ModelResourceManager().Create(cpu, *_staticBatchBuffer, *_skinnedBatchBuffer);
        models.emplace_back(std::move(cpu), std::move(gpu));
    }

    return models;
}
void Renderer::FlushCommands()
{
    GetContext()->VulkanContext()->Device().waitIdle();
}

Renderer::~Renderer()
{
    auto vkContext { _context->VulkanContext() };

    _modelLoader.reset();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkContext->Device().destroy(_inFlightFences[i]);
        vkContext->Device().destroy(_renderFinishedSemaphores[i]);
        vkContext->Device().destroy(_imageAvailableSemaphores[i]);
    }

    _swapChain.reset();

    for (size_t i = 0; i < _tracyContexts.size(); ++i)
    {
        TracyVkDestroy(_tracyContexts[i]);
    }
}

void Renderer::CreateCommandBuffers()
{
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo {};
    commandBufferAllocateInfo.commandPool = _context->VulkanContext()->CommandPool();
    commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
    commandBufferAllocateInfo.commandBufferCount = _commandBuffers.size();

    util::VK_ASSERT(_context->VulkanContext()->Device().allocateCommandBuffers(&commandBufferAllocateInfo, _commandBuffers.data()),
        "Failed allocating command buffer!");
}

void Renderer::RecordCommandBuffer(const vk::CommandBuffer& commandBuffer, uint32_t swapChainImageIndex, float deltaTime)
{
    ZoneScoped;
    TracyVkZone(_tracyContexts[_currentFrame], commandBuffer, "Render all");

    const RenderSceneDescription sceneDescription {
        .gpuScene = _gpuScene,
        .ecs = _ecs,
        .staticBatchBuffer = _staticBatchBuffer,
        .skinnedBatchBuffer = _skinnedBatchBuffer,
        .targetSwapChainImageIndex = swapChainImageIndex,
        .deltaTime = deltaTime,
        .tracyContext = _tracyContexts[_currentFrame],
    };

    _frameGraph->RecordCommands(commandBuffer, _currentFrame, sceneDescription);

    TracyVkCollect(_tracyContexts[_currentFrame], commandBuffer);
}

void Renderer::CreateSyncObjects()
{
    auto vkContext { _context->VulkanContext() };

    vk::SemaphoreCreateInfo semaphoreCreateInfo {};
    vk::FenceCreateInfo fenceCreateInfo {};
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    std::string errorMsg { "Failed creating sync object!" };
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        util::VK_ASSERT(vkContext->Device().createSemaphore(&semaphoreCreateInfo, nullptr, &_imageAvailableSemaphores[i]), errorMsg);
        util::VK_ASSERT(vkContext->Device().createSemaphore(&semaphoreCreateInfo, nullptr, &_renderFinishedSemaphores[i]), errorMsg);
        util::VK_ASSERT(vkContext->Device().createFence(&fenceCreateInfo, nullptr, &_inFlightFences[i]), errorMsg);
    }
}

void Renderer::InitializeHDRTarget()
{
    auto size = _swapChain->GetImageSize();

    CPUImage hdrImageData {};
    hdrImageData.SetName("HDR Target").SetSize(size.x, size.y).SetFormat(vk::Format::eR32G32B32A32Sfloat).SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    _hdrTarget = _context->Resources()->ImageResourceManager().Create(hdrImageData);
}

void Renderer::InitializeBloomTargets()
{
    auto size = _swapChain->GetImageSize();

    CPUImage hdrBloomCreation {};
    hdrBloomCreation.SetName("HDR Bloom Target").SetSize(size.x, size.y).SetFormat(vk::Format::eR16G16B16A16Sfloat).SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    CPUImage hdrBlurredBloomCreation {};
    hdrBlurredBloomCreation.SetName("HDR Blurred Bloom Target").SetSize(size.x, size.y).SetFormat(vk::Format::eR16G16B16A16Sfloat).SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    _brightnessTarget = _context->Resources()->ImageResourceManager().Create(hdrBloomCreation);
    _bloomTarget = _context->Resources()->ImageResourceManager().Create(hdrBlurredBloomCreation);
}
void Renderer::InitializeTonemappingTarget()
{
    auto size = _swapChain->GetImageSize();

    CPUImage tonemappingCreation {};
    tonemappingCreation.SetName("Tonemapping Target").SetSize(size.x, size.y).SetFormat(_swapChain->GetFormat()).SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst);

    _tonemappingTarget = _context->Resources()->ImageResourceManager().Create(tonemappingCreation);
}
void Renderer::InitializeFXAATarget()
{
    auto size = _swapChain->GetImageSize();

    CPUImage fxaaCreation {};
    fxaaCreation.SetName("FXAA Target").SetSize(size.x, size.y).SetFormat(_swapChain->GetFormat()).SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst);

    _fxaaTarget = _context->Resources()->ImageResourceManager().Create(fxaaCreation);
}
void Renderer::InitializeSSAOTarget()
{
    auto size = _swapChain->GetImageSize();

    CPUImage ssaoImageData {};
    ssaoImageData.SetName("SSAO Target")
        .SetSize(size.x, size.y)
        .SetFormat(vk::Format::eR8Unorm)
        .SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    _ssaoTarget = _context->Resources()->ImageResourceManager().Create(ssaoImageData);
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

    CPUImage envMapCreation {};
    envMapCreation.SetSize(width, height).SetFlags(vk::ImageUsageFlagBits::eSampled).SetName("Environment HDRI").SetData(std::move(data)).SetFormat(vk::Format::eR32G32B32A32Sfloat);
    envMapCreation.isHDR = true;

    _environmentMap = _context->Resources()->ImageResourceManager().Create(envMapCreation);
}

void Renderer::UpdateBindless()
{
    _context->UpdateBindlessSet();
}

void Renderer::Render(float deltaTime)
{
    ZoneNamedN(zz, "Renderer::Render()", true);
    {
        ZoneNamedN(zz, "Wait On Fence", true);
        util::VK_ASSERT(_context->VulkanContext()->Device().waitForFences(1, &_inFlightFences[_currentFrame], vk::True, std::numeric_limits<uint64_t>::max()),
            "Failed waiting on in flight fence!");
    }

    _bloomSettings->Update(_currentFrame);
    _viewport.SubmitDrawInfo(_uiPipeline->GetDrawList());
    uint32_t imageIndex {};
    vk::Result result {};

    {
        ZoneNamedN(zz, "Acquire Next Image", true);

        result = _context->VulkanContext()->Device().acquireNextImageKHR(_swapChain->GetSwapChain(), std::numeric_limits<uint64_t>::max(),
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

    util::VK_ASSERT(_context->VulkanContext()->Device().resetFences(1, &_inFlightFences[_currentFrame]), "Failed resetting fences!");

    _commandBuffers[_currentFrame].reset();

    // Since there is only one scene, we can reuse the same gpu buffers
    _gpuScene->Update(_currentFrame);

    _context->GetDrawStats().Clear();

    vk::CommandBufferBeginInfo commandBufferBeginInfo {};
    util::VK_ASSERT(_commandBuffers[_currentFrame].begin(&commandBufferBeginInfo), "Failed to begin recording command buffer!");

    RecordCommandBuffer(_commandBuffers[_currentFrame], imageIndex, deltaTime);

    _commandBuffers[_currentFrame].end();

    vk::Semaphore waitSemaphore = _imageAvailableSemaphores[_currentFrame];
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::Semaphore signalSemaphore = _renderFinishedSemaphores[_currentFrame];

    vk::SubmitInfo submitInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &waitSemaphore,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &_commandBuffers[_currentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &signalSemaphore,
    };

    {
        ZoneNamedN(zz, "Submit Commands", true);
        util::VK_ASSERT(_context->VulkanContext()->GraphicsQueue().submit(1, &submitInfo, _inFlightFences[_currentFrame]), "Failed submitting to graphics queue!");
    }

    vk::SwapchainKHR swapchain = _swapChain->GetSwapChain();
    vk::PresentInfoKHR presentInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &signalSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex,
    };

    {
        ZoneNamedN(zz, "Present Image", true);
        result = _context->VulkanContext()->PresentQueue().presentKHR(&presentInfo);
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

    _context->Resources()->Clean();

    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
