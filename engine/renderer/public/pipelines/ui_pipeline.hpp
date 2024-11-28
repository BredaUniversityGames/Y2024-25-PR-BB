#pragma once

#include "frame_graph.hpp"
#include "gpu_resources.hpp"
#include "quad_draw_info.hpp"
#include <memory>
class SwapChain;
class GraphicsContext;

class UIPipeline final : public FrameGraphRenderPass
{
public:
    UIPipeline(const std::shared_ptr<GraphicsContext>& context, const ResourceHandle<GPUImage>& inputTarget, const ResourceHandle<GPUImage>& outputTarget, const SwapChain& swapChain);
    ~UIPipeline() final;

    NON_COPYABLE(UIPipeline);
    NON_MOVABLE(UIPipeline);
    void RecordCommands(vk::CommandBuffer commandBuffer, MAYBE_UNUSED uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene) final;

    std::vector<QuadDrawInfo>& GetDrawList()
    {
        return _drawList;
    }

private:
    struct UIPushConstants
    {
        QuadDrawInfo quad;
    } _pushConstants;

    std::vector<QuadDrawInfo> _drawList;

    vk::Pipeline _pipeline;
    vk::PipelineLayout _pipelineLayout;

    std::shared_ptr<GraphicsContext> _context;
    ResourceHandle<GPUImage> _inputTarget;
    ResourceHandle<GPUImage> _outputTarget;
    const SwapChain& _swapChain;
    vk::UniqueSampler _sampler;

    void CreatePipeLine();
};
