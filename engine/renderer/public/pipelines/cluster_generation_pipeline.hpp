#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"

class SwapChain;
class GraphicsContext;
struct RenderSceneDescription;

class ClusterGenerationPipeline final : public FrameGraphRenderPass
{
public:
    ClusterGenerationPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const SwapChain& swapChain, ResourceHandle<Buffer>& outputBuffer);
    ~ClusterGenerationPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(ClusterGenerationPipeline);
    NON_COPYABLE(ClusterGenerationPipeline);

private:
    struct PushConstants
    {
        glm::uvec4 tileSizes;
        glm::vec2 screenSize;
        glm::vec2 normPerTileSize;
    } _pushConstants;

    const uint32_t _clusterSizeX = 16;
    const uint32_t _clusterSizeY = 9;
    const uint32_t _clusterSizeZ = 24;
    const uint32_t _numClusters = _clusterSizeX * _clusterSizeY * _clusterSizeZ;
    uint32_t _numTilesX { 0 }, _numTilesY { 0 };

    void CreatePipeline();
    void CreateDescriptorSet();

    std::shared_ptr<GraphicsContext> _context;
    const GBuffers& _gBuffers;
    const SwapChain& _swapChain;

    ResourceHandle<Buffer> _outputBuffer;
    vk::DescriptorSet _outputBufferDescriptorSet;
    vk::DescriptorSetLayout _outputBufferDescriptorSetLayout;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;
};