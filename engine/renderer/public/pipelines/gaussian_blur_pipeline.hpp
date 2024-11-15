#pragma once

#include "constants.hpp"
#include "frame_graph.hpp"
#include "resource_manager.hpp"

#include <cstddef>
#include <memory>

class GraphicsContext;
struct Sampler;

class GaussianBlurPipeline final : public FrameGraphRenderPass
{
public:
    GaussianBlurPipeline(const std::shared_ptr<GraphicsContext>& context, ResourceHandle<Image> source, ResourceHandle<Image> target);
    ~GaussianBlurPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene) final;

    NON_COPYABLE(GaussianBlurPipeline);
    NON_MOVABLE(GaussianBlurPipeline);

private:
    std::shared_ptr<GraphicsContext> _context;

    ResourceHandle<Image> _source;
    std::array<ResourceHandle<Image>, 2> _targets;

    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _sourceDescriptorSets;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _targetDescriptorSets[2];
    ResourceHandle<Sampler> _sampler;

    void CreatePipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();
    void CreateVerticalTarget();
};