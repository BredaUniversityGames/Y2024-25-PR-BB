#pragma once

#include "frame_graph.hpp"

class SwapChain;
class GraphicsContext;
struct RenderSceneDescription;

class ClusterCullingPipeline final : public FrameGraphRenderPass
{
public:
    ClusterCullingPipeline(const std::shared_ptr<GraphicsContext>& context, const SwapChain& swapChain,
        ResourceHandle<Buffer>& clusterBuffer, ResourceHandle<Buffer>& globalIndex,
        ResourceHandle<Buffer>& lightCells, ResourceHandle<Buffer>& lightIndices);
    ~ClusterCullingPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(ClusterCullingPipeline);
    NON_COPYABLE(ClusterCullingPipeline);

private:
    void CreatePipeline();
    void CreateDescriptorSet();

    std::shared_ptr<GraphicsContext> _context;
    const SwapChain& _swapChain;

    ResourceHandle<Buffer> _clusterBuffer;
    ResourceHandle<Buffer> _globalIndex;
    ResourceHandle<Buffer> _lightCells;
    ResourceHandle<Buffer> _lightIndices;

    vk::DescriptorSet _clusterInputDescriptorSet;
    vk::DescriptorSetLayout _clusterInputDescriptorSetLayout;

    vk::DescriptorSet _globalIndexDescriptorSet;
    vk::DescriptorSetLayout _globalIndexDescriptorSetLayout;

    vk::DescriptorSet _lightCellsDescriptorSet;
    vk::DescriptorSetLayout _lightCellsDescriptorSetLayout;

    vk::DescriptorSet _lightIndicesDescriptorSet;
    vk::DescriptorSetLayout _lightIndicesDescriptorSetLayout;

    vk::DescriptorSet _baseDecsriptorSet;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;
};