#pragma once

#include "frame_graph.hpp"

#include <memory>

class BatchBuffer;
class GraphicsContext;
class GPUScene;
class CameraBatch;

struct RenderSceneDescription;

class ShadowPass final : public FrameGraphRenderPass
{
public:
    ShadowPass(const std::shared_ptr<GraphicsContext>& context, const GPUScene& gpuScene, const CameraBatch& cameraBatch);
    ~ShadowPass() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;
    void RequestStaticShadowUpdate(const uint8_t numberOfFrames = 2);

    NON_MOVABLE(ShadowPass);
    NON_COPYABLE(ShadowPass);

private:
    std::shared_ptr<GraphicsContext> _context;
    const CameraBatch& _cameraBatch;

    vk::PipelineLayout _staticPipelineLayout;
    vk::Pipeline _staticPipeline;

    vk::PipelineLayout _skinnedPipelineLayout;
    vk::Pipeline _skinnedPipeline;

    uint8_t _remainingStaticShadowsUpdates = 0;
    void CreateStaticPipeline(const GPUScene& gpuScene);
    void CreateSkinnedPipeline(const GPUScene& gpuScene);

    void DrawGeometry(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene, bool prepass);
};