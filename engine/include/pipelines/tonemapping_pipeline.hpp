#pragma once
#include "swap_chain.hpp"
#include "hdr_target.hpp"

class TonemappingPipeline
{
public:
    TonemappingPipeline(const VulkanBrain& brain, const HDRTarget& hdrTarget, const SwapChain& _swapChain);
    ~TonemappingPipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, uint32_t swapChainIndex);

    NON_COPYABLE(TonemappingPipeline);
    NON_MOVABLE(TonemappingPipeline);

private:
    const VulkanBrain& _brain;
    const SwapChain& _swapChain;
    const HDRTarget& _hdrTarget;

    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _descriptorSets;
    vk::UniqueSampler _sampler;

    void CreatePipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();
};