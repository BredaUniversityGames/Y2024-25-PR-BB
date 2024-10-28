#pragma once

#include "swap_chain.hpp"
#include "application_module.hpp"
#include "mesh.hpp"
#include "camera.hpp"
#include "bloom_settings.hpp"

class DebugPipeline;
class Application;
class GeometryPipeline;
class LightingPipeline;
class SkydomePipeline;
class TonemappingPipeline;
class GaussianBlurPipeline;
class ShadowPipeline;
class IBLPipeline;
class ParticlePipeline;
class SwapChain;
class GBuffers;
class VulkanBrain;
class ModelLoader;
class Engine;
class BatchBuffer;
class ECS;
class GPUScene;
class FrameGraph;

class Renderer
{
public:
    Renderer(ApplicationModule& application_module, const std::shared_ptr<ECS>& ecs);
    ~Renderer();

    NON_COPYABLE(Renderer);
    NON_MOVABLE(Renderer);

    std::vector<std::shared_ptr<ModelHandle>> FrontLoadModels(const std::vector<std::string>& models);

private:
    friend class OldEngine;

    const VulkanBrain _brain;

    std::unique_ptr<ModelLoader> _modelLoader;
    // TODO: Unavoidable currently, this needs to become a module
    ApplicationModule& _application;
    std::shared_ptr<ECS> _ecs;

    std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> _commandBuffers;

    std::unique_ptr<GeometryPipeline> _geometryPipeline;
    std::unique_ptr<LightingPipeline> _lightingPipeline;
    std::unique_ptr<SkydomePipeline> _skydomePipeline;
    std::unique_ptr<TonemappingPipeline> _tonemappingPipeline;
    std::unique_ptr<GaussianBlurPipeline> _bloomBlurPipeline;
    std::unique_ptr<ShadowPipeline> _shadowPipeline;
    std::unique_ptr<DebugPipeline> _debugPipeline;
    std::unique_ptr<IBLPipeline> _iblPipeline;
    std::unique_ptr<ParticlePipeline> _particlePipeline;

    std::shared_ptr<SceneDescription> _scene;
    std::unique_ptr<GPUScene> _gpuScene;
    ResourceHandle<Image> _environmentMap;
    ResourceHandle<Image> _brightnessTarget;
    ResourceHandle<Image> _bloomTarget;

    std::unique_ptr<FrameGraph> _frameGraph;
    std::unique_ptr<SwapChain> _swapChain;
    std::unique_ptr<GBuffers> _gBuffers;

    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> _imageAvailableSemaphores;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> _renderFinishedSemaphores;
    std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> _inFlightFences;

    std::unique_ptr<BatchBuffer> _batchBuffer;

    std::unique_ptr<CameraResource> _camera;

    BloomSettings _bloomSettings;

    ResourceHandle<Image> _hdrTarget;

    uint32_t _currentFrame { 0 };

    void CreateCommandBuffers();
    void RecordCommandBuffer(const vk::CommandBuffer& commandBuffer, uint32_t swapChainImageIndex, float deltaTime);
    void CreateSyncObjects();
    void InitializeHDRTarget();
    void InitializeBloomTargets();
    void LoadEnvironmentMap();
    void UpdateBindless();
    void Render(float deltaTime);
};