#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "generate_draws_pipeline.hpp"

#include <memory>

class BatchBuffer;
class GraphicsContext;

struct RenderSceneDescription;

class ShadowPipeline final : public FrameGraphRenderPass
{
public:
    ShadowPipeline(const std::shared_ptr<GraphicsContext>& context, const GPUScene& gpuScene, const CameraBatch& cameraBatch);
    ~ShadowPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(ShadowPipeline);
    NON_COPYABLE(ShadowPipeline);

private:
    std::shared_ptr<GraphicsContext> _context;
    const CameraBatch& _cameraBatch;

    vk::PipelineLayout _staticPipelineLayout;
    vk::Pipeline _staticPipeline;

    vk::PipelineLayout _skinnedPipelineLayout;
    vk::Pipeline _skinnedPipeline;

    void CreateStaticPipeline(const GPUScene& gpuScene);
    void CreateSkinnedPipeline(const GPUScene& gpuScene);

    void DrawGeometry(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene, bool prepass);
};