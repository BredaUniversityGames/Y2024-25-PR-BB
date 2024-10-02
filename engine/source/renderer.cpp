#include "renderer.hpp"

#include <utility>

#include "vulkan_validation.hpp"
#include "vulkan_helper.hpp"
#include "imgui_impl_vulkan.h"
#include "stopwatch.hpp"
#include "model_loader.hpp"
#include "util.hpp"
#include "mesh_primitives.hpp"
#include "pipelines/geometry_pipeline.hpp"
#include "pipelines/lighting_pipeline.hpp"
#include "pipelines/skydome_pipeline.hpp"
#include "pipelines/tonemapping_pipeline.hpp"
#include "pipelines/gaussian_blur_pipeline.hpp"
#include "pipelines/ibl_pipeline.hpp"
#include "pipelines/shadow_pipeline.hpp"
#include "gbuffers.hpp"
#include "application.hpp"
#include "engine.hpp"
#include "single_time_commands.hpp"
#include "ui/UserInterfaceSystem.hpp"

Renderer::Renderer(const InitInfo& initInfo, const std::shared_ptr<Application>& application)
    : _brain(initInfo)
    , _application(application)
    , _bloomSettings(_brain)
{

    _application->InitImGui();

    _swapChain = std::make_unique<SwapChain>(_brain, glm::uvec2 { initInfo.width, initInfo.height });

    CreateDescriptorSetLayout();
    InitializeCameraUBODescriptors();
    InitializeHDRTarget();
    InitializeBloomTargets();
    LoadEnvironmentMap();
    _uiPipeLine = std::make_unique<UIPipeLine>(_brain, *_swapChain);
    _uiPipeLine->CreatePipeLine();

    m_UIRenderContext = std::make_unique<UserInterfaceRenderContext>();
    m_UIRenderContext->InitializeDefaultRenderSystems(*_uiPipeLine);

    _modelLoader = std::make_unique<ModelLoader>(_brain, _materialDescriptorSetLayout);

    SingleTimeCommands commandBufferPrimitive { _brain };
    MeshPrimitiveHandle uvSphere = _modelLoader->LoadPrimitive(GenerateUVSphere(32, 32), commandBufferPrimitive);
    commandBufferPrimitive.Submit();

    _gBuffers = std::make_unique<GBuffers>(_brain, _swapChain->GetImageSize());
    _geometryPipeline = std::make_unique<GeometryPipeline>(_brain, *_gBuffers, _materialDescriptorSetLayout, _cameraStructure);
    _skydomePipeline = std::make_unique<SkydomePipeline>(_brain, std::move(uvSphere), _cameraStructure, _hdrTarget, _brightnessTarget, _environmentMap, _bloomSettings);
    _tonemappingPipeline = std::make_unique<TonemappingPipeline>(_brain, _hdrTarget, _bloomTarget, *_swapChain, _bloomSettings);
    _bloomBlurPipeline = std::make_unique<GaussianBlurPipeline>(_brain, _brightnessTarget, _bloomTarget);
    _iblPipeline = std::make_unique<IBLPipeline>(_brain, _environmentMap);
    _shadowPipeline = std::make_unique<ShadowPipeline>(_brain, *_gBuffers, _cameraStructure, *_geometryPipeline);
    _lightingPipeline = std::make_unique<LightingPipeline>(_brain, *_gBuffers, _hdrTarget, _brightnessTarget, _cameraStructure, _iblPipeline->IrradianceMap(),
        _iblPipeline->PrefilterMap(), _iblPipeline->BRDFLUTMap(), _bloomSettings);

    SingleTimeCommands commandBufferIBL { _brain };
    _iblPipeline->RecordCommands(commandBufferIBL.CommandBuffer());
    commandBufferIBL.Submit();

    CreateCommandBuffers();
    CreateSyncObjects();
}
Renderer::~Renderer()
{
    _modelLoader.reset();

    _brain.ImageResourceManager().Destroy(_environmentMap);
    _brain.ImageResourceManager().Destroy(_hdrTarget);

    _brain.ImageResourceManager().Destroy(_brightnessTarget);
    _brain.ImageResourceManager().Destroy(_bloomTarget);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        _brain.device.destroy(_inFlightFences[i]);
        _brain.device.destroy(_renderFinishedSemaphores[i]);
        _brain.device.destroy(_imageAvailableSemaphores[i]);
    }

    for (auto& model : _scene->models)
    {
        for (auto& mesh : model->meshes)
        {
            for (auto& primitive : mesh->primitives)
            {
                vmaDestroyBuffer(_brain.vmaAllocator, primitive.vertexBuffer, primitive.vertexBufferAllocation);
                vmaDestroyBuffer(_brain.vmaAllocator, primitive.indexBuffer, primitive.indexBufferAllocation);
            }
        }
        for (auto& texture : model->textures)
        {
            _brain.ImageResourceManager().Destroy(texture);
        }
        for (auto& material : model->materials)
        {
            vmaDestroyBuffer(_brain.vmaAllocator, material->materialUniformBuffer, material->materialUniformAllocation);
        }
    }

    _brain.device.destroy(_cameraStructure.descriptorSetLayout);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vmaUnmapMemory(_brain.vmaAllocator, _cameraStructure.allocations[i]);
        vmaDestroyBuffer(_brain.vmaAllocator, _cameraStructure.buffers[i], _cameraStructure.allocations[i]);
    }

    _swapChain.reset();

    _brain.device.destroy(_materialDescriptorSetLayout);
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

void Renderer::RecordCommandBuffer(const vk::CommandBuffer& commandBuffer, uint32_t swapChainImageIndex)
{
    ZoneScoped;
    const Image* hdrImage = _brain.ImageResourceManager().Access(_hdrTarget);
    const Image* hdrBloomImage = _brain.ImageResourceManager().Access(_brightnessTarget);
    const Image* hdrBlurredBloomImage = _brain.ImageResourceManager().Access(_bloomTarget);
    const Image* shadowMap = _brain.ImageResourceManager().Access(_gBuffers->Shadow());

    vk::CommandBufferBeginInfo commandBufferBeginInfo {};
    util::VK_ASSERT(commandBuffer.begin(&commandBufferBeginInfo), "Failed to begin recording command buffer!");

    util::TransitionImageLayout(commandBuffer, _swapChain->GetImage(swapChainImageIndex), _swapChain->GetFormat(),
        vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
    util::TransitionImageLayout(commandBuffer, hdrImage->image, hdrImage->format, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal);
    _gBuffers->TransitionLayout(commandBuffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

    util::TransitionImageLayout(commandBuffer, shadowMap->image, shadowMap->format, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal, 1, 0, 1, vk::ImageAspectFlagBits::eDepth);
    _shadowPipeline->RecordCommands(commandBuffer, _currentFrame, *_scene);
    _geometryPipeline->RecordCommands(commandBuffer, _currentFrame, *_scene);

    _gBuffers->TransitionLayout(commandBuffer, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    util::TransitionImageLayout(commandBuffer, hdrBloomImage->image, hdrBloomImage->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
    util::TransitionImageLayout(commandBuffer, shadowMap->image, shadowMap->format, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 1, 0, 1, vk::ImageAspectFlagBits::eDepth);

    _skydomePipeline->RecordCommands(commandBuffer, _currentFrame);
    _lightingPipeline->RecordCommands(commandBuffer, _currentFrame);

    util::TransitionImageLayout(commandBuffer, shadowMap->image, shadowMap->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilReadOnlyOptimal, 1, 0, 1, vk::ImageAspectFlagBits::eDepth);
    util::TransitionImageLayout(commandBuffer, hdrBloomImage->image, hdrBloomImage->format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    util::TransitionImageLayout(commandBuffer, hdrBlurredBloomImage->image, hdrBlurredBloomImage->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

    _bloomBlurPipeline->RecordCommands(commandBuffer, _currentFrame, 5);

    util::TransitionImageLayout(commandBuffer, hdrImage->image, hdrImage->format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    util::TransitionImageLayout(commandBuffer, hdrBlurredBloomImage->image, hdrBlurredBloomImage->format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    glm::mat4 projection = glm::ortho(0, int(1920), 0, int(1200));

    _tonemappingPipeline->RecordCommands(commandBuffer, _currentFrame, swapChainImageIndex);
    RenderUI(m_UIElementToRender.get(), *m_UIRenderContext, commandBuffer, _brain, *_swapChain, swapChainImageIndex, projection);

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

void Renderer::CreateDescriptorSetLayout()
{
    auto materialLayoutBindings = MaterialHandle::GetLayoutBindings();
    vk::DescriptorSetLayoutCreateInfo materialCreateInfo {};
    materialCreateInfo.bindingCount = materialLayoutBindings.size();
    materialCreateInfo.pBindings = materialLayoutBindings.data();
    util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&materialCreateInfo, nullptr, &_materialDescriptorSetLayout),
        "Failed creating material descriptor set layout!");

    vk::DescriptorSetLayoutBinding cameraUBODescriptorSetBinding {};
    cameraUBODescriptorSetBinding.binding = 0;
    cameraUBODescriptorSetBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    cameraUBODescriptorSetBinding.descriptorCount = 1;
    cameraUBODescriptorSetBinding.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

    vk::DescriptorSetLayoutCreateInfo cameraUBOCreateInfo {};
    cameraUBOCreateInfo.bindingCount = 1;
    cameraUBOCreateInfo.pBindings = &cameraUBODescriptorSetBinding;
    util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&cameraUBOCreateInfo, nullptr, &_cameraStructure.descriptorSetLayout),
        "Failed creating camera UBO descriptor set layout!");
}

void Renderer::InitializeCameraUBODescriptors()
{
    vk::DeviceSize bufferSize = sizeof(CameraUBO);

    // Create buffers.
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        util::CreateBuffer(_brain, bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            _cameraStructure.buffers[i], true, _cameraStructure.allocations[i],
            VMA_MEMORY_USAGE_CPU_ONLY,
            "Uniform buffer");

        util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, _cameraStructure.allocations[i], &_cameraStructure.mappedPtrs[i]), "Failed mapping memory for UBO!");
    }

    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _cameraStructure.descriptorSetLayout; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = _brain.descriptorPool;
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocateInfo.pSetLayouts = layouts.data();

    util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, _cameraStructure.descriptorSets.data()),
        "Failed allocating descriptor sets!");

    for (size_t i = 0; i < _cameraStructure.descriptorSets.size(); ++i)
    {
        UpdateCameraDescriptorSet(i);
    }
}

void Renderer::UpdateCameraDescriptorSet(uint32_t currentFrame)
{
    vk::DescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = _cameraStructure.buffers[currentFrame];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(CameraUBO);

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

    vk::WriteDescriptorSet& bufferWrite { descriptorWrites[0] };
    bufferWrite.dstSet = _cameraStructure.descriptorSets[currentFrame];
    bufferWrite.dstBinding = 0;
    bufferWrite.dstArrayElement = 0;
    bufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    bufferWrite.descriptorCount = 1;
    bufferWrite.pBufferInfo = &bufferInfo;

    _brain.device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

CameraUBO Renderer::CalculateCamera(const Camera& camera)
{
    CameraUBO ubo {};

    glm::mat4 cameraRotation = glm::mat4_cast(glm::quat(camera.euler_rotation));
    glm::mat4 cameraTranslation = glm::translate(glm::mat4 { 1.0f }, camera.position);

    ubo.view = glm::inverse(cameraTranslation * cameraRotation);

    ubo.proj = glm::perspective(camera.fov, _gBuffers->Size().x / static_cast<float>(_gBuffers->Size().y), camera.nearPlane,
        camera.farPlane);
    ubo.proj[1][1] *= -1;

    ubo.VP = ubo.proj * ubo.view;
    ubo.cameraPosition = camera.position;

    const DirectionalLight& light = _scene->directionalLight;

    const glm::mat4 lightView = glm::lookAt(light.targetPos - normalize(light.lightDir) * light.sceneDistance, light.targetPos, glm::vec3(0, 1, 0));
    glm::mat4 depthProjectionMatrix = glm::ortho<float>(-light.orthoSize, light.orthoSize, -light.orthoSize, light.orthoSize, light.nearPlane, light.farPlane);
    depthProjectionMatrix[1][1] *= -1;
    ubo.lightVP = depthProjectionMatrix * lightView;
    ubo.depthBiasMVP = light.biasMatrix * ubo.lightVP;
    ubo.lightData = glm::vec4(light.targetPos - normalize(light.lightDir) * light.sceneDistance, light.shadowBias); // save light direction here
    return ubo;
}

void Renderer::InitializeHDRTarget()
{
    auto size = _swapChain->GetImageSize();

    ImageCreation hdrCreation {};
    hdrCreation.SetName("HDR Target").SetSize(size.x, size.y).SetFormat(vk::Format::eR32G32B32A32Sfloat).SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    _hdrTarget = _brain.ImageResourceManager().Create(hdrCreation);
}

void Renderer::InitializeBloomTargets()
{
    auto size = _swapChain->GetImageSize();

    ImageCreation hdrBloomCreation {};
    hdrBloomCreation.SetName("HDR Bloom Target").SetSize(size.x, size.y).SetFormat(vk::Format::eR16G16B16A16Sfloat).SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    static auto sampler = util::CreateSampler(_brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToBorder, vk::SamplerMipmapMode::eNearest, 0);
    hdrBloomCreation.sampler = sampler.get();
    ImageCreation hdrBlurredBloomCreation {};
    hdrBlurredBloomCreation.sampler = sampler.get();
    hdrBlurredBloomCreation.SetName("HDR Blurred Bloom Target").SetSize(size.x, size.y).SetFormat(vk::Format::eR16G16B16A16Sfloat).SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    _brightnessTarget = _brain.ImageResourceManager().Create(hdrBloomCreation);
    _bloomTarget = _brain.ImageResourceManager().Create(hdrBlurredBloomCreation);
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

    _environmentMap = _brain.ImageResourceManager().Create(envMapCreation);
}
void Renderer::UpdateCamera(const Camera& camera)
{
    CameraUBO cameraUBO = CalculateCamera(camera);
    std::memcpy(_cameraStructure.mappedPtrs[_currentFrame], &cameraUBO, sizeof(CameraUBO));
}
void Renderer::UpdateBindless()
{
    _brain.UpdateBindlessSet();
}
void Renderer::Render()
{
    ZoneNamedN(zz, "Renderer::Render()", true);

    _bloomSettings.Update(_currentFrame);

    {
        ZoneNamedN(zz, "Wait On Fence", true);
        util::VK_ASSERT(_brain.device.waitForFences(1, &_inFlightFences[_currentFrame], vk::True, std::numeric_limits<uint64_t>::max()),
            "Failed waiting on in flight fence!");
    }

    uint32_t imageIndex {};
    vk::Result result {};

    {
        ZoneNamedN(zz, "Acquire Next Image", true);

        result = _brain.device.acquireNextImageKHR(_swapChain->GetSwapChain(), std::numeric_limits<uint64_t>::max(),
            _imageAvailableSemaphores[_currentFrame], nullptr, &imageIndex);

        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
        {
            _swapChain->Resize(_application->DisplaySize());
            _gBuffers->Resize(_application->DisplaySize());

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

    RecordCommandBuffer(_commandBuffers[_currentFrame], imageIndex);

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

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || _swapChain->GetImageSize() != _application->DisplaySize())
    {
        _swapChain->Resize(_application->DisplaySize());
        _gBuffers->Resize(_application->DisplaySize());
    }
    else
    {
        util::VK_ASSERT(result, "Failed acquiring next image from swap chain!");
    }

    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}