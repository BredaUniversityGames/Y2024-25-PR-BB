#pragma once
#include "frame_graph.hpp"

class SwapChain;
class GraphicsContext;

class PresentationPass final : public FrameGraphRenderPass
{
public:
    PresentationPass(const std::shared_ptr<GraphicsContext>& context, const SwapChain& swapChain, ResourceHandle<GPUImage> input);
    void RecordCommands(vk::CommandBuffer commandBuffer, MAYBE_UNUSED uint32_t currentFrame, const RenderSceneDescription& scene) final;

private:
    std::shared_ptr<GraphicsContext> _context;
    const SwapChain& _swapChain;
    ResourceHandle<GPUImage> _input;
};