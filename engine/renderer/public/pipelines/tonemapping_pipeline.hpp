#pragma once

#include "frame_graph.hpp"
#include "swap_chain.hpp"

#include <cstddef>
#include <memory>

class BloomSettings;
class GraphicsContext;

class TonemappingPipeline final : public FrameGraphRenderPass
{
public:
    TonemappingPipeline(const std::shared_ptr<GraphicsContext>& context, ResourceHandle<GPUImage> hdrTarget, ResourceHandle<GPUImage> bloomTarget, const SwapChain& _swapChain, const BloomSettings& bloomSettings);
    ~TonemappingPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_COPYABLE(TonemappingPipeline);
    NON_MOVABLE(TonemappingPipeline);

private:
    struct PushConstants
    {
        uint32_t hdrTargetIndex;
        uint32_t bloomTargetIndex;
    } _pushConstants;

    std::shared_ptr<GraphicsContext> _context;
    const SwapChain& _swapChain;
    ResourceHandle<GPUImage> _hdrTarget;
    ResourceHandle<GPUImage> _bloomTarget;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    const BloomSettings& _bloomSettings;

    void CreatePipeline();
};