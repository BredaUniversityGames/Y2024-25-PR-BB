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
        // Register 0 (16 bytes)
        uint32_t hdrTargetIndex;
        uint32_t bloomTargetIndex;
        uint32_t depthIndex;
        uint32_t enableFlags;

        // Register 1 (16 bytes)
        uint32_t normalRIndex;
        uint32_t tonemappingFunction;
        uint32_t padding0; // added padding for alignment
        float exposure;

        // Register 2 (16 bytes)
        float vignetteIntensity;
        float lensDistortionIntensity;
        float lensDistortionCubicIntensity;
        float screenScale;

        // Register 3 (16 bytes)
        float brightness;
        float contrast;
        float saturation;
        float vibrance;

        // Register 4 (16 bytes)
        float hue;
        float minPixelSize;
        float maxPixelSize;
        float pixelizationLevels;

        // Register 5 (16 bytes)
        float pixelizationDepthBias;
        uint32_t screenWidth;
        uint32_t screenHeight;
        float ditherAmount;

        // Register 6 (16 bytes)
        float paletteAmount;
        float time;
        float padding1;
        int paletteSize;

        glm::vec4 skyColor;
        glm::vec4 sunColor;
        glm::vec4 cloudsColor;
        glm::vec4 voidColor;

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

    ResourceHandle<Buffer> _colorPaletteBuffer;
    vk::DescriptorSetLayout _paletteDescriptorSetLayout;
    vk::DescriptorSet _paletteDescriptorSet;
    const int _maxColorsInPalette = 64;

    const BloomSettings& _bloomSettings;
    float timePassed = 0.0f;

    void CreatePipeline();
    void UpdatePaletteBuffer(const std::vector<glm::vec4>& paletteColors);
    void CreatePaletteBuffer();
    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();
};