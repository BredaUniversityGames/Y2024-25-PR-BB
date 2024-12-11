#pragma once

#include "application_module.hpp"
#include "bloom_settings.hpp"
#include "cpu_resources.hpp"
#include "ecs_module.hpp"
#include "swap_chain.hpp"

class UIModule;
class DebugPipeline;
class Application;
class GeometryPipeline;
class SSAOPipeline;
class LightingPipeline;
class SkydomePipeline;
class TonemappingPipeline;
class FXAAPipeline;
class UIPipeline;
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
class GPUScene;
class FrameGraph;
class Viewport;

class Renderer
{
public:
    Renderer(ApplicationModule& applicationModule, Viewport& viewport, const std::shared_ptr<GraphicsContext>& context, ECSModule& ecs);
    ~Renderer();

    NON_COPYABLE(Renderer);
    NON_MOVABLE(Renderer);

    void Render(float deltaTime);

    std::vector<std::pair<CPUModel, ResourceHandle<GPUModel>>> FrontLoadModels(const std::vector<std::string>& modelPaths);

    ModelLoader& GetModelLoader() const { return *_modelLoader; }
    BatchBuffer& StaticBatchBuffer() const { return *_skinnedBatchBuffer; }
    BatchBuffer& SkinnedBatchBuffer() const { return *_staticBatchBuffer; }
    SwapChain& GetSwapChain() const { return *_swapChain; }
    GBuffers& GetGBuffers() const { return *_gBuffers; }
    std::shared_ptr<GraphicsContext> GetContext() const { return _context; }
    DebugPipeline& GetDebugPipeline() const { return *_debugPipeline; }
    BloomSettings& GetBloomSettings() { return *_bloomSettings; }
    SSAOPipeline& GetSSAOPipeline() const { return *_ssaoPipeline; }
    FXAAPipeline& GetFXAAPipeline() const { return *_fxaaPipeline; }

private:
    friend class RendererModule;
    std::shared_ptr<GraphicsContext> _context;

    std::unique_ptr<ModelLoader> _modelLoader;

    // TODO: Unavoidable currently, this needs to become a module
    ApplicationModule& _application;
    Viewport& _viewport;
    ECSModule& _ecs;

    std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> _commandBuffers;

    std::unique_ptr<GeometryPipeline> _geometryPipeline;
    std::unique_ptr<LightingPipeline> _lightingPipeline;
    std::unique_ptr<SkydomePipeline> _skydomePipeline;
    std::unique_ptr<TonemappingPipeline> _tonemappingPipeline;
    std::unique_ptr<FXAAPipeline> _fxaaPipeline;
    std::unique_ptr<UIPipeline> _uiPipeline;
    std::unique_ptr<GaussianBlurPipeline> _bloomBlurPipeline;
    std::unique_ptr<ShadowPipeline> _shadowPipeline;
    std::unique_ptr<DebugPipeline> _debugPipeline;
    std::unique_ptr<IBLPipeline> _iblPipeline;
    std::unique_ptr<ParticlePipeline> _particlePipeline;
    std::unique_ptr<SSAOPipeline> _ssaoPipeline;

    std::shared_ptr<GPUScene> _gpuScene;
    ResourceHandle<GPUImage> _environmentMap;
    ResourceHandle<GPUImage> _brightnessTarget;
    ResourceHandle<GPUImage> _bloomTarget;
    ResourceHandle<GPUImage> _tonemappingTarget;
    ResourceHandle<GPUImage> _fxaaTarget;
    ResourceHandle<GPUImage> _uiTarget;

    std::unique_ptr<FrameGraph> _frameGraph;
    std::unique_ptr<SwapChain> _swapChain;
    std::unique_ptr<GBuffers> _gBuffers;

    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> _imageAvailableSemaphores;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> _renderFinishedSemaphores;
    std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> _inFlightFences;

    std::shared_ptr<BatchBuffer> _staticBatchBuffer;
    std::shared_ptr<BatchBuffer> _skinnedBatchBuffer;

    std::unique_ptr<BloomSettings> _bloomSettings;

    ResourceHandle<GPUImage> _hdrTarget;
    ResourceHandle<GPUImage> _ssaoTarget;

    uint32_t _currentFrame { 0 };

    void CreateCommandBuffers();
    void RecordCommandBuffer(const vk::CommandBuffer& commandBuffer, uint32_t swapChainImageIndex, float deltaTime);
    void CreateSyncObjects();
    void InitializeHDRTarget();
    void InitializeBloomTargets();
    void InitializeTonemappingTarget();
    void InitializeFXAATarget();
    void InitializeUITarget();
    void InitializeSSAOTarget();
    void LoadEnvironmentMap();
    void UpdateBindless();
};