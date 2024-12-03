#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"

class SwapChain;
class GraphicsContext;
struct RenderSceneDescription;

class ClusteringPipeline final : public FrameGraphRenderPass
{
public:
    ClusteringPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers);
    ~ClusteringPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(ClusteringPipeline);
    NON_COPYABLE(ClusteringPipeline);

private:
    struct PushConstants
    {
        glm::uvec4 tileSizes;
        glm::vec2 screenSize;
    } _pushConstants;

    void CreatePipeline();

    std::shared_ptr<GraphicsContext> _context;
    const GBuffers& _gBuffers;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;
};