#pragma once
#include "frame_graph.hpp"

struct Image;

class BloomDownsamplePass final : public FrameGraphRenderPass
{
public:
    BloomDownsamplePass(const std::shared_ptr<GraphicsContext>& context, ResourceHandle<GPUImage> bloomImage);
    ~BloomDownsamplePass() final;
    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

private:
    std::shared_ptr<GraphicsContext> _context;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    ResourceHandle<GPUImage> _bloomImage;

    void CreatPipeline();
};
