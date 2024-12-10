#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "indirect_culler.hpp"

#include <memory>

class BatchBuffer;
class GraphicsContext;

struct RenderSceneDescription;

class ShadowPipeline final : public FrameGraphRenderPass
{
public:
    ShadowPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const GPUScene& gpuScene);
    ~ShadowPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(ShadowPipeline);
    NON_COPYABLE(ShadowPipeline);

private:
    void CreateStaticPipeline();
    void CreateSkinnedPipeline();

    void CreateDrawBufferDescriptorSet(const GPUScene& gpuScene);

    std::shared_ptr<GraphicsContext> _context;
    const GBuffers& _gBuffers;

    IndirectCuller _culler;

    vk::PipelineLayout _staticPipelineLayout;
    vk::Pipeline _staticPipeline;

    vk::PipelineLayout _skinnedPipelineLayout;
    vk::Pipeline _skinnedPipeline;

    ResourceHandle<Buffer> _drawBuffer;
    vk::DescriptorSet _drawBufferDescriptorSet;
};