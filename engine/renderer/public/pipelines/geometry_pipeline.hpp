#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "indirect_culler.hpp"

#include <memory>

class BatchBuffer;
class GPUScene;
class GraphicsContext;

struct RenderSceneDescription;

class GeometryPipeline final : public FrameGraphRenderPass
{
public:
    GeometryPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, ResourceHandle<GPUImage> hzbImage, const GPUScene& gpuScene);
    ~GeometryPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(GeometryPipeline);
    NON_COPYABLE(GeometryPipeline);

private:
    std::shared_ptr<GraphicsContext> _context;
    const GBuffers& _gBuffers;

    std::unique_ptr<IndirectCuller> _culler;

    vk::PipelineLayout _staticPipelineLayout;
    vk::Pipeline _staticPipeline;

    vk::PipelineLayout _skinnedPipelineLayout;
    vk::Pipeline _skinnedPipeline;

    vk::PipelineLayout _buildHzbPipelineLayout;
    vk::Pipeline _buildHzbPipeline;
    vk::DescriptorSetLayout _hzbImageDSL;
    vk::DescriptorUpdateTemplate _hzbUpdateTemplate;
    ResourceHandle<GPUImage> _hzbImage;

    ResourceHandle<Buffer> _drawBuffer;
    vk::DescriptorSet _drawBufferDescriptorSet;

    ResourceHandle<Buffer> _visibilityBuffer;
    vk::DescriptorSetLayout _visibilityDSL;
    vk::DescriptorSet _visibilityDescriptorSet;

    ResourceHandle<Sampler> _hzbSampler;

    void CreateStaticPipeline();
    void CreateSkinnedPipeline();
    void CreatBuildHzbPipeline();
    void CreateDrawBufferDescriptorSet(const GPUScene& gpuScene);

    void BuildHzb(const RenderSceneDescription& scene, vk::CommandBuffer commandBuffer);
};
