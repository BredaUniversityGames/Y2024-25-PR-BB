#pragma once

#include "swap_chain.hpp"
#include "engine_init_info.hpp"
#include "performance_tracker.hpp"
#include "mesh.hpp"
#include "camera.hpp"
#include "pipelines/shadow_pipeline.hpp"
#include "bloom_settings.hpp"

class Application;
class GeometryPipeline;
class LightingPipeline;
class SkydomePipeline;
class TonemappingPipeline;
class GaussianBlurPipeline;
class IBLPipeline;
class SwapChain;
class GBuffers;
class VulkanBrain;
class ModelLoader;

class Engine
{
public:
    Engine(const InitInfo& initInfo, std::shared_ptr<Application> application);
    ~Engine();
    NON_COPYABLE(Engine);
    NON_MOVABLE(Engine);

    void Run();
    bool ShouldQuit() const { return _shouldQuit; };
    void Quit() { _shouldQuit = true; };

private:
    const VulkanBrain _brain;
    vk::DescriptorSetLayout _materialDescriptorSetLayout;
    std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> _commandBuffers;

    std::unique_ptr<GeometryPipeline> _geometryPipeline;
    std::unique_ptr<LightingPipeline> _lightingPipeline;
    std::unique_ptr<SkydomePipeline> _skydomePipeline;
    std::unique_ptr<ShadowPipeline> _shadowPipeline;
    std::unique_ptr<TonemappingPipeline> _tonemappingPipeline;
    std::unique_ptr<GaussianBlurPipeline> _bloomBlurPipeline;
    std::unique_ptr<IBLPipeline> _iblPipeline;
    std::unique_ptr<ModelLoader> _modelLoader;

    SceneDescription _scene;
    ResourceHandle<Image> _environmentMap;

    std::unique_ptr<SwapChain> _swapChain;
    std::unique_ptr<GBuffers> _gBuffers;

    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> _imageAvailableSemaphores;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> _renderFinishedSemaphores;
    std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> _inFlightFences;

    CameraStructure _cameraStructure;

    ResourceHandle<Image> _hdrTarget;
    ResourceHandle<Image> _brightnessTarget;
    ResourceHandle<Image> _bloomTarget;

    std::shared_ptr<Application> _application;

    glm::ivec2 _lastMousePos;

    uint32_t _currentFrame { 0 };
    std::chrono::time_point<std::chrono::high_resolution_clock> _lastFrameTime;

    PerformanceTracker _performanceTracker;

    BloomSettings _bloomSettings;

    bool _shouldQuit = false;

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
};
