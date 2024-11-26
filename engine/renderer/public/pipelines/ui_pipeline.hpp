#pragma once
#include "frame_graph.hpp"
#include "swap_chain.hpp"

class GraphicsContext;

struct alignas(16) QuadDrawInfo
{
    alignas(16) glm::mat4 projection;
    alignas(16) glm::vec4 color = { 1.f, 1.f, 1.f, 1.f };
    alignas(8) glm::vec2 uvp1 = { 0.f, 0.f };
    alignas(8) glm::vec2 uvp2 = { 1.f, 1.f };
    alignas(4) uint32_t textureIndex;
    alignas(4) uint32_t useRedAsAlpha = false;
};

struct UIPushConstants
{
    QuadDrawInfo quad;
};

class UIPipeline final : public FrameGraphRenderPass
{
public:
    UIPipeline(const std::shared_ptr<GraphicsContext>& context, const SwapChain& swapChain);
    ~UIPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_COPYABLE(UIPipeline);
    NON_MOVABLE(UIPipeline);

    std::vector<QuadDrawInfo> _drawlist;

private:
    void CreatePipeLine();

    std::shared_ptr<GraphicsContext> _context;
    const SwapChain& _swapChain;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    vk::UniqueSampler _sampler;
};
