#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "geometry_pipeline.hpp"
#include "mesh.hpp"

#include <memory>

class SwapChain;
class BatchBuffer;
class GraphicsContext;

class DebugPipeline final : public FrameGraphRenderPass
{
public:
    DebugPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const SwapChain& swapChain);
    ~DebugPipeline() final;

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

    // enable or disable drawing debug lines
    void SetState(const bool newState) { _isEnabled = newState; }
    bool GetState() const { return _isEnabled; }

    NON_MOVABLE(DebugPipeline);
    NON_COPYABLE(DebugPipeline);

private:
    std::shared_ptr<GraphicsContext> _context;
    const GBuffers& _gBuffers;
    const SwapChain& _swapChain;
    bool _isEnabled = true;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    std::vector<glm::vec3> _linesData;
    ResourceHandle<Buffer> _vertexBuffer;

    void CreatePipeline();
    void CreateVertexBuffer();
    void UpdateVertexData();
};