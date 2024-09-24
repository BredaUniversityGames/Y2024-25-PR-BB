#include "engine.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
#include "gbuffers.hpp"
#include "application.hpp"
#include "single_time_commands.hpp"

Engine::Engine(const InitInfo& initInfo, std::shared_ptr<Application> application) :
    _brain(initInfo)
{
    auto path = std::filesystem::current_path();
    spdlog::info("Current path: {}", path.string());

    ImGui::CreateContext();
    ImPlot::CreateContext();
    spdlog::info("Starting engine...");

    _application = std::move(application);

    _swapChain = std::make_unique<SwapChain>(_brain, glm::uvec2{ initInfo.width, initInfo.height });

    CreateDescriptorSetLayout();
    InitializeCameraUBODescriptors();
    InitializeHDRTarget();
    InitializeBloomTargets();
    LoadEnvironmentMap();

    _modelLoader = std::make_unique<ModelLoader>(_brain, _materialDescriptorSetLayout);

    SingleTimeCommands commandBufferPrimitive{ _brain };
    MeshPrimitiveHandle uvSphere = _modelLoader->LoadPrimitive(GenerateUVSphere(32, 32), commandBufferPrimitive);
    commandBufferPrimitive.Submit();

    _gBuffers = std::make_unique<GBuffers>(_brain, _swapChain->GetImageSize());
    _geometryPipeline = std::make_unique<GeometryPipeline>(_brain, *_gBuffers, _materialDescriptorSetLayout, _cameraStructure);
    _skydomePipeline = std::make_unique<SkydomePipeline>(_brain, std::move(uvSphere), _cameraStructure, _hdrTarget, _environmentMap);
    _tonemappingPipeline = std::make_unique<TonemappingPipeline>(_brain, _hdrTarget, _hdrBlurredBloomTarget, *_swapChain);
    _bloomBlurPipeline = std::make_unique<GaussianBlurPipeline>(_brain, _hdrBloomTarget, _hdrBlurredBloomTarget);
    _iblPipeline = std::make_unique<IBLPipeline>(_brain, _environmentMap);
    _lightingPipeline = std::make_unique<LightingPipeline>(_brain, *_gBuffers, _hdrTarget, _hdrBloomTarget, _cameraStructure, _iblPipeline->IrradianceMap(), _iblPipeline->PrefilterMap(), _iblPipeline->BRDFLUTMap());

    SingleTimeCommands commandBufferIBL{ _brain };
    _iblPipeline->RecordCommands(commandBufferIBL.CommandBuffer());
    commandBufferIBL.Submit();

    CreateCommandBuffers();
    CreateSyncObjects();

    _scene.models.emplace_back(std::make_shared<ModelHandle>(_modelLoader->Load("assets/models/DamagedHelmet.glb")));
    _scene.models.emplace_back(std::make_shared<ModelHandle>(_modelLoader->Load("assets/models/ABeautifulGame/ABeautifulGame.gltf")));

    glm::vec3 scale{0.05f};
    glm::mat4 rotation{glm::quat(glm::vec3(0.0f, 90.0f, 0.0f))};
    glm::vec3 translate{-0.275f, 0.06f, -0.025f};
    glm::mat4 transform = glm::translate(glm::mat4{1.0f}, translate) * rotation * glm::scale(glm::mat4{1.0f}, scale);

    _scene.gameObjects.emplace_back(transform, _scene.models[0]);
    _scene.gameObjects.emplace_back(glm::mat4{1.0f}, _scene.models[1]);

    vk::Format format = _swapChain->GetFormat();
    vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfoKhr{};
    pipelineRenderingCreateInfoKhr.colorAttachmentCount = 1;
    pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = &format;
    pipelineRenderingCreateInfoKhr.depthAttachmentFormat = _gBuffers->DepthFormat();

    _application->InitImGui();

    ImGui_ImplVulkan_InitInfo initInfoVulkan{};
    initInfoVulkan.UseDynamicRendering = true;
    initInfoVulkan.PipelineRenderingCreateInfo = static_cast<VkPipelineRenderingCreateInfo>(pipelineRenderingCreateInfoKhr);
    initInfoVulkan.PhysicalDevice = _brain.physicalDevice;
    initInfoVulkan.Device = _brain.device;
    initInfoVulkan.ImageCount = MAX_FRAMES_IN_FLIGHT;
    initInfoVulkan.Instance = _brain.instance;
    initInfoVulkan.MSAASamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
    initInfoVulkan.Queue = _brain.graphicsQueue;
    initInfoVulkan.QueueFamily = _brain.queueFamilyIndices.graphicsFamily.value();
    initInfoVulkan.DescriptorPool = _brain.descriptorPool;
    initInfoVulkan.MinImageCount = 2;
    initInfoVulkan.ImageCount = _swapChain->GetImageCount();
    ImGui_ImplVulkan_Init(&initInfoVulkan);

    ImGui_ImplVulkan_CreateFontsTexture();

    _scene.camera.position = glm::vec3{ 0.0f, 0.2f, 0.0f };
    _scene.camera.fov = glm::radians(45.0f);
    _scene.camera.nearPlane = 0.01f;
    _scene.camera.farPlane = 100.0f;

    _lastFrameTime = std::chrono::high_resolution_clock::now();

    glm::ivec2 mousePos;
    _application->GetInputManager().GetMousePosition(mousePos.x, mousePos.y);
    _lastMousePos = mousePos;

    _application->SetMouseHidden(true);

    spdlog::info("Successfully initialized engine!");
}

void Engine::Run()
{
    ZoneNamed(zone, "");
    auto currentFrameTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> deltaTime = currentFrameTime - _lastFrameTime;
    _lastFrameTime = currentFrameTime;
    float deltaTimeMS = deltaTime.count();

    // Slow down application when minimized.
    if(_application->IsMinimized())
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(16ms);
        return;
    }

    {
        ZoneNamedN(zone, "Update Camera", true);
        int x, y;
        _application->GetInputManager().GetMousePosition(x, y);

        glm::ivec2 mouse_delta = glm::ivec2(x, y) - _lastMousePos;
        _lastMousePos = { x, y };
 
        constexpr float MOUSE_SENSITIVITY = 0.003f;
        constexpr float CAM_SPEED = 0.003f;

        constexpr glm::vec3 RIGHT = { 1.0f, 0.0f, 0.0f};
        constexpr glm::vec3 FORWARD = { 0.0f, 0.0f, 1.0f };
        //constexpr glm::vec3 UP = { 0.0f, -1.0f, 0.0f };

        _scene.camera.euler_rotation.x -= mouse_delta.y * MOUSE_SENSITIVITY;
        _scene.camera.euler_rotation.y -= mouse_delta.x * MOUSE_SENSITIVITY;
  
        glm::vec3 movement_dir{};
        if (_application->GetInputManager().IsKeyHeld(InputManager::Key::W))
            movement_dir -= FORWARD;

        if (_application->GetInputManager().IsKeyHeld(InputManager::Key::S))
            movement_dir += FORWARD;

        if (_application->GetInputManager().IsKeyHeld(InputManager::Key::D))
            movement_dir += RIGHT;

        if (_application->GetInputManager().IsKeyHeld(InputManager::Key::A))
            movement_dir -= RIGHT;

        if (glm::length(movement_dir) != 0.0f)
        {
            movement_dir = glm::normalize(movement_dir);
        }

        _scene.camera.position += glm::quat(_scene.camera.euler_rotation) * movement_dir * deltaTimeMS * CAM_SPEED;
    }

    if (_application->GetInputManager().IsKeyPressed(InputManager::Key::Escape))
        Quit();

    CameraUBO cameraUBO = CalculateCamera(_scene.camera);
    std::memcpy(_cameraStructure.mappedPtrs[_currentFrame], &cameraUBO, sizeof(CameraUBO));
    
    {
        ZoneNamedN(zone, "Wait On Fence", true);
        util::VK_ASSERT(_brain.device.waitForFences(1, &_inFlightFences[_currentFrame], vk::True, std::numeric_limits<uint64_t>::max()),
                    "Failed waiting on in flight fence!");
    }

    uint32_t imageIndex{};
    vk::Result result{};

    {
        ZoneNamedN(zone, "Acquire Next Image", true);

        result = _brain.device.acquireNextImageKHR(_swapChain->GetSwapChain(), std::numeric_limits<uint64_t>::max(),
                                                        _imageAvailableSemaphores[_currentFrame], nullptr, &imageIndex);

        if(result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
        {
            _swapChain->Resize(_application->DisplaySize());
            _gBuffers->Resize(_application->DisplaySize());

            return;
        } else
            util::VK_ASSERT(result, "Failed acquiring next image from swap chain!");
    }

    util::VK_ASSERT(_brain.device.resetFences(1, &_inFlightFences[_currentFrame]), "Failed resetting fences!");

    ImGui_ImplVulkan_NewFrame();
    _application->NewImGuiFrame();
    ImGui::NewFrame();

    _performanceTracker.Render();
    
    {
        ZoneNamedN(zone, "ImGui Render", true);
        ImGui::Render();
    }

    _commandBuffers[_currentFrame].reset();

    RecordCommandBuffer(_commandBuffers[_currentFrame], imageIndex);

    vk::SubmitInfo submitInfo{};
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
        ZoneNamedN(zone, "Submit Commands", true);
        util::VK_ASSERT(_brain.graphicsQueue.submit(1, &submitInfo, _inFlightFences[_currentFrame]), "Failed submitting to graphics queue!");
    }

    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    vk::SwapchainKHR swapchains[] = { _swapChain->GetSwapChain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    
    {
        ZoneNamedN(zone, "Present Image", true);
        result = _brain.presentQueue.presentKHR(&presentInfo);
    }

    if(result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || _swapChain->GetImageSize() != _application->DisplaySize())
    {
        _swapChain->Resize(_application->DisplaySize());
        _gBuffers->Resize(_application->DisplaySize());
    }
    else
    {
        util::VK_ASSERT(result, "Failed acquiring next image from swap chain!");
    }

    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    _performanceTracker.Update();
}

Engine::~Engine()
{
    _brain.device.waitIdle();

    ImGui_ImplVulkan_Shutdown();
    _application->ShutdownImGui();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    _brain.ImageResourceManager().Destroy(_environmentMap);
    _brain.ImageResourceManager().Destroy(_hdrTarget);

    _brain.ImageResourceManager().Destroy(_hdrBloomTarget);
    _brain.ImageResourceManager().Destroy(_hdrBlurredBloomTarget);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        _brain.device.destroy(_inFlightFences[i]);
        _brain.device.destroy(_renderFinishedSemaphores[i]);
        _brain.device.destroy(_imageAvailableSemaphores[i]);
    }

    for(auto& model : _scene.models)
    {
        for(auto& mesh : model->meshes)
        {
            for(auto& primitive : mesh->primitives)
            {
                vmaDestroyBuffer(_brain.vmaAllocator, primitive.vertexBuffer, primitive.vertexBufferAllocation);
                vmaDestroyBuffer(_brain.vmaAllocator, primitive.indexBuffer, primitive.indexBufferAllocation);
            }
        }
        for(auto& texture : model->textures)
        {
            _brain.ImageResourceManager().Destroy(texture);
        }
        for(auto& material : model->materials)
        {
            vmaDestroyBuffer(_brain.vmaAllocator, material->materialUniformBuffer, material->materialUniformAllocation);
        }
    }

    _brain.device.destroy(_cameraStructure.descriptorSetLayout);
    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vmaUnmapMemory(_brain.vmaAllocator, _cameraStructure.allocations[i]);
        vmaDestroyBuffer(_brain.vmaAllocator, _cameraStructure.buffers[i], _cameraStructure.allocations[i]);
    }

    _swapChain.reset();

    _brain.device.destroy(_materialDescriptorSetLayout);
}

void Engine::CreateCommandBuffers()
{
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.commandPool = _brain.commandPool;
    commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
    commandBufferAllocateInfo.commandBufferCount = _commandBuffers.size();

    util::VK_ASSERT(_brain.device.allocateCommandBuffers(&commandBufferAllocateInfo, _commandBuffers.data()),
                    "Failed allocating command buffer!");
}

void Engine::RecordCommandBuffer(const vk::CommandBuffer &commandBuffer, uint32_t swapChainImageIndex)
{
    ZoneScoped;
    const Image* hdrImage = _brain.ImageResourceManager().Access(_hdrTarget);
    const Image* hdrBloomImage = _brain.ImageResourceManager().Access(_hdrBloomTarget);
    const Image* hdrBlurredBloomImage = _brain.ImageResourceManager().Access(_hdrBlurredBloomTarget);

    vk::CommandBufferBeginInfo commandBufferBeginInfo{};
    util::VK_ASSERT(commandBuffer.begin(&commandBufferBeginInfo), "Failed to begin recording command buffer!");

    util::TransitionImageLayout(commandBuffer, _swapChain->GetImage(swapChainImageIndex), _swapChain->GetFormat(), vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
    util::TransitionImageLayout(commandBuffer, hdrImage->image, hdrImage->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
    _gBuffers->TransitionLayout(commandBuffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

    _geometryPipeline->RecordCommands(commandBuffer, _currentFrame, _scene);

    _gBuffers->TransitionLayout(commandBuffer, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    util::TransitionImageLayout(commandBuffer, hdrBloomImage->image, hdrBloomImage->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

    _skydomePipeline->RecordCommands(commandBuffer, _currentFrame);
    _lightingPipeline->RecordCommands(commandBuffer, _currentFrame);

    util::TransitionImageLayout(commandBuffer, hdrBloomImage->image, hdrBloomImage->format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    util::TransitionImageLayout(commandBuffer, hdrBlurredBloomImage->image, hdrBlurredBloomImage->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

    _bloomBlurPipeline->RecordCommands(commandBuffer, _currentFrame, 5);

    util::TransitionImageLayout(commandBuffer, hdrImage->image, hdrImage->format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    util::TransitionImageLayout(commandBuffer, hdrBlurredBloomImage->image, hdrBlurredBloomImage->format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    _tonemappingPipeline->RecordCommands(commandBuffer, _currentFrame, swapChainImageIndex);

    util::TransitionImageLayout(commandBuffer, _swapChain->GetImage(swapChainImageIndex), _swapChain->GetFormat(), vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);

    commandBuffer.end();
}

void Engine::CreateSyncObjects()
{
    vk::SemaphoreCreateInfo semaphoreCreateInfo{};
    vk::FenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    std::string errorMsg{ "Failed creating sync object!" };
    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        util::VK_ASSERT(_brain.device.createSemaphore(&semaphoreCreateInfo, nullptr, &_imageAvailableSemaphores[i]), errorMsg);
        util::VK_ASSERT(_brain.device.createSemaphore(&semaphoreCreateInfo, nullptr, &_renderFinishedSemaphores[i]), errorMsg);
        util::VK_ASSERT(_brain.device.createFence(&fenceCreateInfo, nullptr, &_inFlightFences[i]), errorMsg);
    }
}

void Engine::CreateDescriptorSetLayout()
{
    auto materialLayoutBindings = MaterialHandle::GetLayoutBindings();
    vk::DescriptorSetLayoutCreateInfo materialCreateInfo{};
    materialCreateInfo.bindingCount = materialLayoutBindings.size();
    materialCreateInfo.pBindings = materialLayoutBindings.data();
    util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&materialCreateInfo, nullptr, &_materialDescriptorSetLayout),
                    "Failed creating material descriptor set layout!");


    vk::DescriptorSetLayoutBinding cameraUBODescriptorSetBinding{};
    cameraUBODescriptorSetBinding.binding = 0;
    cameraUBODescriptorSetBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    cameraUBODescriptorSetBinding.descriptorCount = 1;
    cameraUBODescriptorSetBinding.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

    vk::DescriptorSetLayoutCreateInfo cameraUBOCreateInfo{};
    cameraUBOCreateInfo.bindingCount = 1;
    cameraUBOCreateInfo.pBindings = &cameraUBODescriptorSetBinding;
    util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&cameraUBOCreateInfo, nullptr, &_cameraStructure.descriptorSetLayout),
                    "Failed creating camera UBO descriptor set layout!");
}

void Engine::InitializeCameraUBODescriptors()
{
    vk::DeviceSize bufferSize = sizeof(CameraUBO);

    // Create buffers.
    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        util::CreateBuffer(_brain, bufferSize,
                           vk::BufferUsageFlagBits::eUniformBuffer,
                           _cameraStructure.buffers[i], true, _cameraStructure.allocations[i],
                           VMA_MEMORY_USAGE_CPU_ONLY,
                           "Uniform buffer");

        util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, _cameraStructure.allocations[i], &_cameraStructure.mappedPtrs[i]), "Failed mapping memory for UBO!");
    }

    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts{};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
    { l = _cameraStructure.descriptorSetLayout; });
    vk::DescriptorSetAllocateInfo allocateInfo{};
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

void Engine::UpdateCameraDescriptorSet(uint32_t currentFrame)
{
    vk::DescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = _cameraStructure.buffers[currentFrame];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(CameraUBO);

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites{};

    vk::WriteDescriptorSet& bufferWrite{ descriptorWrites[0] };
    bufferWrite.dstSet = _cameraStructure.descriptorSets[currentFrame];
    bufferWrite.dstBinding = 0;
    bufferWrite.dstArrayElement = 0;
    bufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    bufferWrite.descriptorCount = 1;
    bufferWrite.pBufferInfo = &bufferInfo;

    _brain.device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

CameraUBO Engine::CalculateCamera(const Camera& camera)
{
    CameraUBO ubo{};

    glm::mat4 cameraRotation = glm::mat4_cast(glm::quat(camera.euler_rotation));
    glm::mat4 cameraTranslation = glm::translate(glm::mat4{1.0f}, camera.position);

    ubo.view = glm::inverse(cameraTranslation * cameraRotation);

    ubo.proj = glm::perspective(camera.fov, _gBuffers->Size().x / static_cast<float>(_gBuffers->Size().y), camera.nearPlane, camera.farPlane);
    ubo.proj[1][1] *= -1;

    ubo.VP = ubo.proj * ubo.view;
    ubo.cameraPosition = camera.position;

    return ubo;
}

void Engine::InitializeHDRTarget()
{
    auto size = _swapChain->GetImageSize();

    ImageCreation hdrCreation{};
    hdrCreation.SetName("HDR Target").SetSize(size.x, size.y).SetFormat(vk::Format::eR32G32B32A32Sfloat).SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    _hdrTarget = _brain.ImageResourceManager().Create(hdrCreation);
}

void Engine::InitializeBloomTargets()
{
    auto size = _swapChain->GetImageSize();

    ImageCreation hdrBloomCreation{};
    hdrBloomCreation.SetName("HDR Bloom Target").SetSize(size.x, size.y).SetFormat(vk::Format::eR32G32B32A32Sfloat).SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    ImageCreation hdrBlurredBloomCreation{};
    hdrBlurredBloomCreation.SetName("HDR Blurred Bloom Target").SetSize(size.x, size.y).SetFormat(vk::Format::eR32G32B32A32Sfloat).SetFlags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    _hdrBloomTarget = _brain.ImageResourceManager().Create(hdrBloomCreation);
    _hdrBlurredBloomTarget = _brain.ImageResourceManager().Create(hdrBlurredBloomCreation);
}

void Engine::LoadEnvironmentMap()
{
    int32_t width, height, numChannels;
    float* stbiData = stbi_loadf("assets/hdri/industrial_sunset_02_puresky_4k.hdr", &width, &height, &numChannels, 4);

    if(stbiData == nullptr)
        throw std::runtime_error("Failed loading HDRI!");

    std::vector<std::byte> data(width * height * 4 * sizeof(float));
    std::memcpy(data.data(), stbiData, data.size());

    stbi_image_free(stbiData);

    ImageCreation envMapCreation{};
    envMapCreation.SetSize(width, height).SetFlags(vk::ImageUsageFlagBits::eSampled).SetName("Environment HDRI").SetData(data.data()).SetFormat(vk::Format::eR32G32B32A32Sfloat);
    envMapCreation.isHDR = true;

    _environmentMap = _brain.ImageResourceManager().Create(envMapCreation);
}
