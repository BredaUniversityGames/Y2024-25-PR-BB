#pragma once
#include "swap_chain.hpp"
#include "vulkan_brain.hpp"

class BloomSettings;

class TonemappingPipeline
{
public:
    TonemappingPipeline(const VulkanBrain& brain, ResourceHandle<Image> hdrTarget, ResourceHandle<Image> bloomTarget, const SwapChain& _swapChain, const BloomSettings& bloomSettings);
    ~TonemappingPipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, uint32_t swapChainIndex);

    NON_COPYABLE(TonemappingPipeline);
    NON_MOVABLE(TonemappingPipeline);

private:
    struct PushConstants
    {
        uint32_t hdrTargetIndex;
        uint32_t bloomTargetIndex;
    } _pushConstants;

    const VulkanBrain& _brain;
    const SwapChain& _swapChain;
    ResourceHandle<Image> _hdrTarget;
    ResourceHandle<Image> _bloomTarget;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    const BloomSettings& _bloomSettings;

    void CreatePipeline();
};