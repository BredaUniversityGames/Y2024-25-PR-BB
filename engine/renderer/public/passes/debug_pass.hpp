﻿#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "vertex.hpp"

#include <memory>

class SwapChain;
class BatchBuffer;
class GraphicsContext;

class DebugPass final : public FrameGraphRenderPass
{
public:
    DebugPass(const std::shared_ptr<GraphicsContext>& context, const SwapChain& swapChain, const GBuffers& gBuffers, ResourceHandle<GPUImage> attachment);
    ~DebugPass() final;

    void AddLines(const std::vector<glm::vec3>& linesData)
    {
        _linesData.insert(_linesData.end(), linesData.begin(), linesData.end());
    }

    void AddLine(const glm::vec3& start, const glm::vec3& end)
    {
        _linesData.push_back(start);
        _linesData.push_back(end);
    }

    void ClearLines() { _linesData.clear(); }
    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(DebugPass);
    NON_COPYABLE(DebugPass);

private:
    std::shared_ptr<GraphicsContext> _context;
    const SwapChain& _swapChain;
    const GBuffers& _gBuffers;
    ResourceHandle<GPUImage> _attachment;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    std::vector<glm::vec3> _linesData;
    ResourceHandle<Buffer> _vertexBuffer;

    void CreatePipeline();
    void CreateVertexBuffer();
    void UpdateVertexData();
};