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

class GenerateDrawsPass final : public FrameGraphRenderPass
{
public:
    GenerateDrawsPass(const std::shared_ptr<GraphicsContext>& context, const CameraBatch& cameraBatch);
    ~GenerateDrawsPass() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_COPYABLE(GenerateDrawsPass);
    NON_MOVABLE(GenerateDrawsPass);

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
