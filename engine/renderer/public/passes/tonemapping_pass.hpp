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
    TonemappingPass(const std::shared_ptr<GraphicsContext>& context, const Settings::Tonemapping& settings, ResourceHandle<GPUImage> hdrTarget, ResourceHandle<GPUImage> bloomTarget, ResourceHandle<GPUImage> outputTarget, const SwapChain& _swapChain, const GBuffers& gBuffers, const BloomSettings& bloomSettings);
    ~TonemappingPass() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_COPYABLE(TonemappingPass);
    NON_MOVABLE(TonemappingPass);

private:
    enum TonemappingFlags : uint32_t
    {
        eEnableVignette = 1 << 0,
        eEnableLensDistortion = 1 << 1,
        eEnableToneAdjustments = 1 << 2,
        eEnablePixelization = 1 << 3,
        eEnablePalette = 1 << 4,
        // Add more flags as needed.
    };

    struct PushConstants
    {
        uint32_t hdrTargetIndex;
        uint32_t bloomTargetIndex;
        uint32_t depthIndex;
        uint32_t enableFlags;

        uint32_t tonemappingFunction { 0 };
        float exposure { 1.0f };

        float vignetteIntensity;
        float lensDistortionIntensity;
        float lensDistortionCubicIntensity;
        float screenScale;

        float brightness;
        float contrast;
        float saturation;
        float vibrance;
        float hue;

        float minPixelSize;
        float maxPixelSize;
        float pixelizationLevels;
        float pixelizationDepthBias;
        uint32_t screenWidth;
        uint32_t screenHeight;

        float ditherAmount;
        float paletteAmount;
        uint32_t pad0;

        glm::vec4 palette[5];
    } _pushConstants;

    std::shared_ptr<GraphicsContext> _context;
    const Settings::Tonemapping& _settings;
    const SwapChain& _swapChain;
    const GBuffers& _gBuffers;
    ResourceHandle<GPUImage> _hdrTarget;
    ResourceHandle<GPUImage> _bloomTarget;
    ResourceHandle<GPUImage> _outputTarget;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    const BloomSettings& _bloomSettings;

    void CreatePipeline();
};