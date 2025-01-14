#pragma once

#include "constants.hpp"
#include "frame_graph.hpp"
#include "resource_manager.hpp"

#include <cstddef>
#include <memory>

class GraphicsContext;
struct Sampler;

class GaussianBlurPass final : public FrameGraphRenderPass
{
public:
    GaussianBlurPass(const std::shared_ptr<GraphicsContext>& context, ResourceHandle<GPUImage> source, ResourceHandle<GPUImage> target);
    ~GaussianBlurPass() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene) final;

    NON_COPYABLE(GaussianBlurPass);
    NON_MOVABLE(GaussianBlurPass);

private:
    std::shared_ptr<GraphicsContext> _context;

    ResourceHandle<GPUImage> _source;
    std::array<ResourceHandle<GPUImage>, 2> _targets;

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