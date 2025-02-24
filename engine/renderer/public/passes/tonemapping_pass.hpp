#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "settings.hpp"
#include "swap_chain.hpp"

#include <cstdint>
#include <memory>

class BloomSettings;
class GraphicsContext;

class TonemappingPass final : public FrameGraphRenderPass
{
public:
    TonemappingPass(const std::shared_ptr<GraphicsContext>& context, const Settings::Tonemapping& settings, ResourceHandle<GPUImage> hdrTarget, ResourceHandle<GPUImage> bloomTarget, const GBuffers& gBuffers, ResourceHandle<GPUImage> outputTarget, const SwapChain& _swapChain, const BloomSettings& bloomSettings);
    ~TonemappingPass() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_COPYABLE(TonemappingPass);
    NON_MOVABLE(TonemappingPass);

private:
    struct PushConstants
    {
        uint32_t hdrTargetIndex;
        uint32_t bloomTargetIndex;
        uint32_t depthIndex;

        uint32_t tonemappingFunction { 0 };
        float exposure { 1.0f };

        uint32_t enableVignette;
        float vignetteIntensity;

        uint32_t enableLensDistortion;
        float lensDistortionIntensity;
        float lensDistortionCubicIntensity;
        float screenScale;

        uint32_t enableToneAdjustments;
        float brightness;
        float contrast;
        float saturation;
        float vibrance;
        float hue;
    } _pushConstants;

    std::shared_ptr<GraphicsContext> _context;
    const Settings::Tonemapping& _settings;
    const SwapChain& _swapChain;
    ResourceHandle<GPUImage> _hdrTarget;
    ResourceHandle<GPUImage> _bloomTarget;
    ResourceHandle<GPUImage> _outputTarget;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    const BloomSettings& _bloomSettings;
    const GBuffers& _gBuffers;

    void CreatePipeline();
};