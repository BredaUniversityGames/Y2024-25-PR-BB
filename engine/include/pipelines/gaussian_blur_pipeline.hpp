#pragma once

#include <vulkan_brain.hpp>
#include <resource_manager.hpp>
#include "constants.hpp"
#include "frame_graph.hpp"

class GaussianBlurPipeline : public FrameGraphRenderPass
{
public:
    GaussianBlurPipeline(const VulkanBrain& brain, ResourceHandle<Image> source, ResourceHandle<Image> target);
    ~GaussianBlurPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene) final;

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