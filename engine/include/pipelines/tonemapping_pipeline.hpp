#pragma once
#include "swap_chain.hpp"

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
    const VulkanBrain& _brain;
    const SwapChain& _swapChain;
    ResourceHandle<Image> _hdrTarget;
    ResourceHandle<Image> _bloomTarget;

    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _descriptorSets;
    vk::UniqueSampler _sampler;

    const BloomSettings& _bloomSettings;

    void CreatePipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();
};