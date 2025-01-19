#pragma once

#include "frame_graph.hpp"
#include "swap_chain.hpp"

#include <cstdint>
#include <memory>

class BloomSettings;
class GraphicsContext;

class TonemappingPass final : public FrameGraphRenderPass
{
public:
    TonemappingPass(const std::shared_ptr<GraphicsContext>& context, ResourceHandle<GPUImage> hdrTarget, ResourceHandle<GPUImage> bloomTarget, ResourceHandle<GPUImage> outputTarget, const SwapChain& _swapChain, const BloomSettings& bloomSettings);
    ~TonemappingPass() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_COPYABLE(TonemappingPass);
    NON_MOVABLE(TonemappingPass);

private:
    struct PushConstants
    {
        uint32_t hdrTargetIndex;
        uint32_t bloomTargetIndex;

        uint32_t tonemappingFunction { 0 };
        float exposure { 1.0f };
        float vignetteIntensity;
    } _pushConstants;

    std::shared_ptr<GraphicsContext> _context;
    const SwapChain& _swapChain;
    ResourceHandle<GPUImage> _hdrTarget;
    ResourceHandle<GPUImage> _bloomTarget;
    ResourceHandle<GPUImage> _outputTarget;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    const BloomSettings& _bloomSettings;

    void CreatePipeline();
};