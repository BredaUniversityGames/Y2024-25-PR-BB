#pragma once
#include "include.hpp"
#include "swap_chain.hpp"

class TonemappingPipeline
{
public:
    TonemappingPipeline(const VulkanBrain& brain, const ImageHandle hdrTarget, const SwapChain& _swapChain);
    ~TonemappingPipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, uint32_t swapChainIndex);

    NON_COPYABLE(TonemappingPipeline);
    NON_MOVABLE(TonemappingPipeline);

private:
    const VulkanBrain& _brain;
    const SwapChain& _swapChain;
    const ImageHandle _hdrTarget;

    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _descriptorSets;
    vk::UniqueSampler _sampler;

    void CreatePipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();
};