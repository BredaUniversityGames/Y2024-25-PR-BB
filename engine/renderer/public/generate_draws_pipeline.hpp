#pragma once

#include "common.hpp"
#include "frame_graph.hpp"
#include "gpu_resources.hpp"

#include <cstdint>
#include <memory>

class GPUScene;
class GraphicsContext;
class CameraBatch;
struct RenderSceneDescription;

class GenerateDrawsPipeline final : public FrameGraphRenderPass
{
public:
    GenerateDrawsPipeline(const std::shared_ptr<GraphicsContext>& context, const CameraBatch& cameraBatch);
    ~GenerateDrawsPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_COPYABLE(GenerateDrawsPipeline);
    NON_MOVABLE(GenerateDrawsPipeline);

private:
    std::shared_ptr<GraphicsContext> _context;

    vk::PipelineLayout _generateDrawsPipelineLayout;
    vk::Pipeline _generateDrawsPipeline;

    bool _isPrepass = true;

    const CameraBatch& _cameraBatch;

    struct PushConstants
    {
        uint32_t isPrepass;
        float mipSize;
        uint32_t hzbIndex;
        uint32_t drawCommandsCount;
    } _pushConstants;

    void CreateCullingPipeline();
};
