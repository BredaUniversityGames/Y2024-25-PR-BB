#pragma once
#include "frame_graph.hpp"

struct Image;

class BloomUpsamplePass final : public FrameGraphRenderPass
{
public:
    BloomUpsamplePass(const std::shared_ptr<GraphicsContext>& context, ResourceHandle<GPUImage> bloomImage);
    ~BloomUpsamplePass() final;
    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

private:
    std::shared_ptr<GraphicsContext> _context;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    ResourceHandle<GPUImage> _bloomImage;

    void CreatPipeline();
};
