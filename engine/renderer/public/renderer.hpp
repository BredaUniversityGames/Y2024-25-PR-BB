#pragma once

#include "application_module.hpp"
#include "bloom_settings.hpp"
#include "camera.hpp"
#include "entt/entity/entity.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "swap_chain.hpp"

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
class GraphicsContext;
class ModelLoader;
class Engine;
class BatchBuffer;
class ECS;
class GPUScene;
class FrameGraph;

class Renderer
{
public:
    Renderer(ApplicationModule& application_module, const std::shared_ptr<GraphicsContext>& context, const std::shared_ptr<ECS>& ecs);
    ~Renderer();

    NON_COPYABLE(Renderer);
    NON_MOVABLE(Renderer);

    void Render(float deltaTime);

    std::vector<Model> FrontLoadModels(const std::vector<std::string>& modelPaths);

    ModelLoader& GetModelLoader() const { return *_modelLoader; }
    BatchBuffer& GetBatchBuffer() const { return *_batchBuffer; }
    SwapChain& GetSwapChain() const { return *_swapChain; }
    GBuffers& GetGBuffers() const { return *_gBuffers; }
    std::shared_ptr<GraphicsContext> GetContext() const { return _context; }
    DebugPipeline& GetDebugPipeline() const { return *_debugPipeline; }
    BloomSettings& GetBloomSettings() { return *_bloomSettings; }

private:
    friend class RendererModule;
    std::shared_ptr<GraphicsContext> _context;

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

    std::shared_ptr<const SceneDescription> _scene;
    std::shared_ptr<GPUScene> _gpuScene;
    ResourceHandle<Image> _environmentMap;
    ResourceHandle<Image> _brightnessTarget;
    ResourceHandle<Image> _bloomTarget;

    std::unique_ptr<FrameGraph> _frameGraph;
    std::unique_ptr<SwapChain> _swapChain;
    std::unique_ptr<GBuffers> _gBuffers;

    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> _imageAvailableSemaphores;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> _renderFinishedSemaphores;
    std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> _inFlightFences;

    std::shared_ptr<BatchBuffer> _batchBuffer;

    std::unique_ptr<CameraResource> _camera;

    std::unique_ptr<BloomSettings> _bloomSettings;

    ResourceHandle<Image> _hdrTarget;

    uint32_t _currentFrame { 0 };

    void CreateCommandBuffers();
    void RecordCommandBuffer(const vk::CommandBuffer& commandBuffer, uint32_t swapChainImageIndex, float deltaTime);
    void CreateSyncObjects();
    void InitializeHDRTarget();
    void InitializeBloomTargets();
    void LoadEnvironmentMap();
    void UpdateBindless();
};