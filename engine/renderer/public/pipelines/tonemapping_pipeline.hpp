#pragma once
#include "frame_graph.hpp"
#include "swap_chain.hpp"
#include "vulkan_context.hpp"

class BloomSettings;

class TonemappingPipeline final : public FrameGraphRenderPass
{
public:
    TonemappingPipeline(const VulkanContext& brain, ResourceHandle<Image> hdrTarget, ResourceHandle<Image> bloomTarget, const SwapChain& _swapChain, const BloomSettings& bloomSettings);
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

    const VulkanContext& _brain;
    const SwapChain& _swapChain;
    ResourceHandle<Image> _hdrTarget;
    ResourceHandle<Image> _bloomTarget;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    const BloomSettings& _bloomSettings;

    void CreatePipeline();
};