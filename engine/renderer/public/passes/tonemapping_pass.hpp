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
    enum TonemappingFlags : uint32_t
    {
        ENABLE_VIGNETTE = 1 << 0,
        ENABLE_LENS_DISTORTION = 1 << 1,
        ENABLE_TONE_ADJUSTMENTS = 1 << 2,
        ENABLE_PIXELIZATION = 1 << 3,
        ENABLE_PALETTE = 1 << 4,
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

        uint32_t pad0; // offset 84
        uint32_t pad1; // offset 88
        uint32_t pad2; // offset 92

        glm::vec4 palette[5];
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

    // Helper functions to set and query flags.
    inline void setFlag(uint32_t& flags, TonemappingFlags flag, bool enabled)
    {
        if (enabled)
            flags |= flag;
        else
            flags &= ~flag;
    }

    inline bool isFlagSet(uint32_t flags, TonemappingFlags flag)
    {
        return (flags & flag) != 0;
    }
};