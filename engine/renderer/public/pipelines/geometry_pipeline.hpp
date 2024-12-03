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
    GeometryPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const GPUScene& gpuScene);
    ~GeometryPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(GeometryPipeline);
    NON_COPYABLE(GeometryPipeline);

private:
    std::shared_ptr<GraphicsContext> _context;
    const GBuffers& _gBuffers;

    IndirectCuller _culler;

    vk::PipelineLayout _staticPipelineLayout;
    vk::Pipeline _staticPipeline;

    vk::PipelineLayout _skinnedPipelineLayout;
    vk::Pipeline _skinnedPipeline;

    ResourceHandle<Buffer> _drawBuffer;
    vk::DescriptorSet _drawBufferDescriptorSet;

    void CreateStaticPipeline();
    void CreateSkinnedPipeline();
    void CreateDrawBufferDescriptorSet(const GPUScene& gpuScene);
};
