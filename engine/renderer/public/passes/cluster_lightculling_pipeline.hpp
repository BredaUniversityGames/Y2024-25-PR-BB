#pragma once

#include "frame_graph.hpp"

class SwapChain;
class GraphicsContext;
struct RenderSceneDescription;
class GPUScene;

class ClusterLightCullingPipeline final : public FrameGraphRenderPass
{
public:
    ClusterLightCullingPipeline(const std::shared_ptr<GraphicsContext>& context, GPUScene& gpuScene,
        ResourceHandle<Buffer>& clusterBuffer, ResourceHandle<Buffer>& globalIndex,
        ResourceHandle<Buffer>& lightCells, ResourceHandle<Buffer>& lightIndices);
    ~ClusterLightCullingPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(ClusterLightCullingPipeline);
    NON_COPYABLE(ClusterLightCullingPipeline);

private:
    void CreatePipeline();

    std::shared_ptr<GraphicsContext> _context;
    GPUScene& _gpuScene;

    ResourceHandle<Buffer> _clusterBuffer;
    ResourceHandle<Buffer> _globalIndex;
    ResourceHandle<Buffer> _lightCells;
    ResourceHandle<Buffer> _lightIndices;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;
};