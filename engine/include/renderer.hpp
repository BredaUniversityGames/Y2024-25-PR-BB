#pragma once

#include "swap_chain.hpp"
#include "engine_init_info.hpp"
#include "mesh.hpp"
#include "camera.hpp"
#include "bloom_settings.hpp"

class UIPipeLine;
class UserInterfaceRenderContext;
struct UIElement;
class Application;
class GeometryPipeline;
class LightingPipeline;
class SkydomePipeline;
class TonemappingPipeline;
class GaussianBlurPipeline;
class ShadowPipeline;
class IBLPipeline;
class SwapChain;
class GBuffers;
class VulkanBrain;
class ModelLoader;
class Engine;

class Renderer
{
public:
    Renderer(const InitInfo& initInfo, const std::shared_ptr<Application>& application);
    ~Renderer();

    NON_COPYABLE(Renderer);
    NON_MOVABLE(Renderer);

private:
    friend Engine;

    const VulkanBrain _brain;

    std::unique_ptr<ModelLoader> _modelLoader;
    std::shared_ptr<Application> _application;

    vk::DescriptorSetLayout _materialDescriptorSetLayout;
    std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> _commandBuffers;

    std::unique_ptr<GeometryPipeline> _geometryPipeline;
    std::unique_ptr<LightingPipeline> _lightingPipeline;
    std::unique_ptr<SkydomePipeline> _skydomePipeline;
    std::unique_ptr<TonemappingPipeline> _tonemappingPipeline;
    std::unique_ptr<GaussianBlurPipeline> _bloomBlurPipeline;
    std::unique_ptr<ShadowPipeline> _shadowPipeline;
    std::unique_ptr<IBLPipeline> _iblPipeline;

    std::shared_ptr<SceneDescription> _scene;
    ResourceHandle<Image> _environmentMap;
    ResourceHandle<Image> _brightnessTarget;
    ResourceHandle<Image> _bloomTarget;

    std::unique_ptr<SwapChain> _swapChain;
    std::unique_ptr<GBuffers> _gBuffers;

    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> _imageAvailableSemaphores;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> _renderFinishedSemaphores;
    std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> _inFlightFences;

    std::shared_ptr<UIElement> m_UIElementToRender;
    std::unique_ptr<UserInterfaceRenderContext> m_UIRenderContext;

    CameraStructure _cameraStructure;

    BloomSettings _bloomSettings;

    ResourceHandle<Image> _hdrTarget;

    uint32_t _currentFrame { 0 };

    void CreateDescriptorSetLayout();
    void CreateCommandBuffers();
    void RecordCommandBuffer(const vk::CommandBuffer& commandBuffer, uint32_t swapChainImageIndex);
    void CreateSyncObjects();
    void InitializeCameraUBODescriptors();
    void UpdateCameraDescriptorSet(uint32_t currentFrame);
    CameraUBO CalculateCamera(const Camera& camera);
    void InitializeHDRTarget();
    void InitializeBloomTargets();
    void LoadEnvironmentMap();
    void UpdateCamera(const Camera& camera);
    void UpdateBindless();
    void Render();
};