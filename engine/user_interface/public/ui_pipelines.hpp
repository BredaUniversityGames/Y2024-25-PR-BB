#pragma once
#include "../../include/pch.hpp"

class SwapChain;
class VulkanBrain;

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

class UIPipeline
{
public:
    void CreatePipeLine();
    explicit UIPipeline(const VulkanBrain& brain)
        : _brain(brain)
    {
        CreatePipeLine();
    };

    NON_COPYABLE(UIPipeline);
    NON_MOVABLE(UIPipeline);

    void RecordCommands(vk::CommandBuffer commandBuffer, const SwapChain& swapChain, uint32_t swapChainIndex, const glm::mat4& projectionMatrix);

    ~UIPipeline();

    vk::Pipeline _pipeline;
    vk::PipelineLayout _pipelineLayout;

    const VulkanBrain& _brain;
    vk::UniqueSampler _sampler;

    std::vector<QuadDrawInfo> _drawlist;
};
