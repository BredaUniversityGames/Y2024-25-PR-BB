#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"

#include <memory>

class BatchBuffer;
class GPUScene;
class GraphicsContext;
class CameraBatch;

struct RenderSceneDescription;

class GeometryPass final : public FrameGraphRenderPass
{
public:
    GeometryPass(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const CameraBatch& cameraBatch);
    ~GeometryPass() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(GeometryPass);
    NON_COPYABLE(GeometryPass);

private:
    struct PushConstants
    {
        uint32_t isDirectCommand {};
        uint32_t directInstanceIndex {};
    };

    std::shared_ptr<GraphicsContext> _context;
    const GBuffers& _gBuffers;
    const CameraBatch& _cameraBatch;

    vk::PipelineLayout _staticPipelineLayout;
    vk::Pipeline _staticPipeline;

    vk::PipelineLayout _skinnedPipelineLayout;
    vk::Pipeline _skinnedPipeline;

    void CreateStaticPipeline();
    void CreateSkinnedPipeline();

    void DrawGeometry(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene, bool prepass);
    void DrawIndirectGeometry(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene);
    void DrawDirectGeometry(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene);
};
