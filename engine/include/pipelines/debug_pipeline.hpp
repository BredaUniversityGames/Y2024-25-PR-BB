#pragma once

#include "gbuffers.hpp"
#include "mesh.hpp"
#include "geometry_pipeline.hpp"
#include "frame_graph.hpp"

class SwapChain;
class BatchBuffer;

class DebugPipeline final : public FrameGraphRenderPass
{
public:
    DebugPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const CameraResource& camera, const SwapChain& swapChain, const GPUScene& gpuScene);
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

    NON_MOVABLE(DebugPipeline);
    NON_COPYABLE(DebugPipeline);

private:
    void CreatePipeline();

    void CreateVertexBuffer();

    void UpdateVertexData();

    const VulkanBrain& _brain;
    const GBuffers& _gBuffers;
    const SwapChain& _swapChain;
    const CameraResource& _camera;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    std::vector<glm::vec3> _linesData;
    ResourceHandle<Buffer> _vertexBuffer;
};