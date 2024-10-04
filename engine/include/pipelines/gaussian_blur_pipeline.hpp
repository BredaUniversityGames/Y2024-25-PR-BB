#pragma once
#include <vulkan_brain.hpp>
#include <constants.hpp>

class GaussianBlurPipeline
{
public:
    GaussianBlurPipeline(const VulkanBrain& brain, ResourceHandle<Image> source, ResourceHandle<Image> target);
    ~GaussianBlurPipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, uint32_t blurPasses);

    NON_COPYABLE(GaussianBlurPipeline);
    NON_MOVABLE(GaussianBlurPipeline);

private:
    const VulkanBrain& _brain;

    ResourceHandle<Image> _source;
    std::array<ResourceHandle<Image>, 2> _targets;

    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _sourceDescriptorSets;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _targetDescriptorSets[2];
    vk::UniqueSampler _sampler;

    void CreatePipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();
    void CreateVerticalTarget();
};