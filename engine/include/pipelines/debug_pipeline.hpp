#pragma once

#include "gbuffers.hpp"
#include "mesh.hpp"
#include "geometry_pipeline.hpp"

class SwapChain;
class BatchBuffer;

class DebugPipeline
{
public:
    struct FrameData
    {
        vk::Buffer vertexBuffer;
        VmaAllocation vertexBufferAllocation;
        void* vertexBufferMapped;
    };

    DebugPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const CameraStructure& camera, const SwapChain& swapChain, const GPUScene& gpuScene);
    ~DebugPipeline();

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

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, uint32_t swapChainIndex);

    NON_MOVABLE(DebugPipeline);
    NON_COPYABLE(DebugPipeline);

private:
    void CreatePipeline();

    void CreateVertexBuffer();

    void UpdateVertexData(uint32_t currentFrame);

    const VulkanBrain& _brain;
    const GBuffers& _gBuffers;
    const SwapChain& _swapChain;
    const CameraStructure& _camera;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;
    const vk::DescriptorSetLayout& _descriptorSetLayout;

    std::vector<glm::vec3> _linesData;
    // std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _frameData;

    ResourceHandle<Buffer> _vertexBuffer;
};