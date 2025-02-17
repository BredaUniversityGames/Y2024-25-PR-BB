#pragma once

#include "application_module.hpp"
#include "bloom_settings.hpp"
#include "cpu_resources.hpp"
#include "data_store.hpp"
#include "ecs_module.hpp"
#include "settings.hpp"
#include "swap_chain.hpp"

#include <tracy/TracyVulkan.hpp>

class BuildHzbPass;
class GenerateDrawsPass;
class UIModule;
class DebugPass;
class Application;
class GeometryPass;
class SSAOPass;
class LightingPass;
class SkydomePass;
class TonemappingPass;
class FXAAPass;
class UIPass;
class GaussianBlurPass;
class ShadowPass;
class IBLPass;
class ParticlePass;
class PresentationPass;
class SwapChain;
class GBuffers;
class GraphicsContext;
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

    std::vector<ResourceHandle<GPUModel>> LoadModels(const std::vector<CPUModel>& cpuModels);

    BatchBuffer& StaticBatchBuffer() const { return *_skinnedBatchBuffer; }
    BatchBuffer& SkinnedBatchBuffer() const { return *_staticBatchBuffer; }
    SwapChain& GetSwapChain() const { return *_swapChain; }
    GBuffers& GetGBuffers() const { return *_gBuffers; }
    std::shared_ptr<GraphicsContext> GetContext() const { return _context; }
    DebugPass& GetDebugPipeline() const { return *_debugPass; }
    BloomSettings& GetBloomSettings() { return *_bloomSettings; }
    SSAOPass& GetSSAOPipeline() const { return *_ssaoPass; }
    FXAAPass& GetFXAAPipeline() const { return *_fxaaPass; }
    GPUScene& GetGPUScene() { return *_gpuScene; }

    void FlushCommands();

    Settings& GetSettings() { return _settings.data; };

private:
    friend class RendererModule;
    std::shared_ptr<GraphicsContext> _context;

    // TODO: Unavoidable currently, this needs to become a module
    ApplicationModule& _application;
    Viewport& _viewport;
    ECSModule& _ecs;

    DataStore<Settings> _settings;

    std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> _commandBuffers;

    std::unique_ptr<GenerateDrawsPass> _generateMainDrawsPass;
    std::unique_ptr<GenerateDrawsPass> _generateShadowDrawsPass;
    std::unique_ptr<BuildHzbPass> _buildMainHzbPass;
    std::unique_ptr<BuildHzbPass> _buildShadowHzbPass;
    std::unique_ptr<GeometryPass> _geometryPass;
    std::unique_ptr<ShadowPass> _shadowPass;
    std::unique_ptr<LightingPass> _lightingPass;
    std::unique_ptr<SkydomePass> _skydomePass;
    std::unique_ptr<TonemappingPass> _tonemappingPass;
    std::unique_ptr<FXAAPass> _fxaaPass;
    std::unique_ptr<UIPass> _uiPass;
    std::unique_ptr<GaussianBlurPass> _bloomBlurPass;
    std::unique_ptr<DebugPass> _debugPass;
    std::unique_ptr<IBLPass> _iblPass;
    std::unique_ptr<ParticlePass> _particlePass;
    std::unique_ptr<SSAOPass> _ssaoPass;
    std::unique_ptr<PresentationPass> _presentationPass;

    std::shared_ptr<GPUScene> _gpuScene;
    ResourceHandle<GPUImage> _environmentMap;
    ResourceHandle<GPUImage> _brightnessTarget;
    ResourceHandle<GPUImage> _bloomTarget;
    ResourceHandle<GPUImage> _tonemappingTarget;
    ResourceHandle<GPUImage> _fxaaTarget;

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

    std::array<TracyVkCtx, MAX_FRAMES_IN_FLIGHT> _tracyContexts;

    void CreateCommandBuffers();
    void RecordCommandBuffer(const vk::CommandBuffer& commandBuffer, uint32_t swapChainImageIndex, float deltaTime);
    void CreateSyncObjects();
    void InitializeHDRTarget();
    void InitializeBloomTargets();
    void InitializeTonemappingTarget();
    void InitializeFXAATarget();
    void InitializeSSAOTarget();
    void LoadEnvironmentMap();
    void UpdateBindless();
};
