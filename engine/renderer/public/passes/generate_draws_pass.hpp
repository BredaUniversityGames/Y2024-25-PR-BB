#pragma once

#include "camera_batch.hpp"
#include "common.hpp"
#include "frame_graph.hpp"
#include "gpu_resources.hpp"

#include <cstdint>
#include <memory>

class GPUScene;
class GraphicsContext;
struct RenderSceneDescription;

class GenerateDrawsPass final : public FrameGraphRenderPass
{
public:
    GenerateDrawsPass(const std::shared_ptr<GraphicsContext>& context, const CameraBatch& cameraBatch, const bool drawStatic, const bool drawDynamic);
    ~GenerateDrawsPass() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    void SetDrawStatic(bool drawStatic) { _shouldDrawStatic = drawStatic; }
    void SetDrawDynamic(bool drawDynamic) { _shouldDrawDynamic = drawDynamic; }

    NON_COPYABLE(GenerateDrawsPass);
    NON_MOVABLE(GenerateDrawsPass);

private:
    std::shared_ptr<GraphicsContext> _context;

    vk::PipelineLayout _generateDrawsPipelineLayout;
    vk::Pipeline _generateDrawsPipeline;

    bool _isPrepass = true;
    bool _shouldDrawStatic = true;
    bool _shouldDrawDynamic = true;
    const uint32_t _localComputeSize = 64;

    const CameraBatch& _cameraBatch;

    struct PushConstants
    {
        uint32_t isPrepass;
        float mipSize;
        uint32_t hzbIndex;
        uint32_t drawCommandsCount;
        uint32_t isReverseZ;
        uint32_t drawStaticDraws;
    };

    void CreateCullingPipeline();
    void RecordPrepassCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const CameraBatch::Draw& draw, vk::DescriptorSet sceneDraws, vk::DescriptorSet sceneInstances, uint32_t drawCount, const PushConstants& pc);
    void RecordSecondPassCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const CameraBatch::Draw& draw, vk::DescriptorSet sceneDraws, vk::DescriptorSet sceneInstances, uint32_t drawCount, const PushConstants& pc);
};
