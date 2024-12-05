#pragma once

#include "frame_graph.hpp"

class GraphicsContext;
struct RenderSceneDescription;

class ClusterCullingPipeline final : public FrameGraphRenderPass
{
public:
    ClusterCullingPipeline(const std::shared_ptr<GraphicsContext>& context, const SwapChain& swapChain, ResourceHandle<Buffer>& inputBuffer, ResourceHandle<Buffer>& outputBuffer);
    ~ClusterCullingPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(ClusterCullingPipeline);
    NON_COPYABLE(ClusterCullingPipeline);

private:
    void CreatePipeline();
    void CreateDescriptorSet();


    std::shared_ptr<GraphicsContext> _context;
    const SwapChain& _swapChain;

    ResourceHandle<Buffer> _inputBuffer;
    ResourceHandle<Buffer> _outputBuffer;
    vk::DescriptorSet _outputBufferDescriptorSet;
    vk::DescriptorSetLayout _outputBufferDescriptorSetLayout;


    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;
};