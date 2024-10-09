#pragma once

#include "gbuffers.hpp"
#include "mesh.hpp"
#include "geometry_pipeline.hpp"

class SwapChain;
class BatchBuffer;

class PhysicsRenderPipeline
{
public:
    struct FrameData
    {
        vk::Buffer vertexBuffer;
        VmaAllocation vertexBufferAllocation;
        void* vertexBufferMapped;
        vk::DescriptorSet descriptorSet;
    };

    PhysicsRenderPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const CameraStructure& camera, GeometryPipeline& geometryPipeline, const SwapChain& swapChain);
    ~PhysicsRenderPipeline();

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT>& GetFrameData() { return _frameData; }

    void AddLines(const std::vector<glm::vec3>& linesData)
    {
        _linesData.insert(_linesData.end(), linesData.begin(), linesData.end());
    }
    void ClearLinesData() { _linesData.clear(); }

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t, uint32_t swapChainIndex);

    NON_MOVABLE(PhysicsRenderPipeline);
    NON_COPYABLE(PhysicsRenderPipeline);

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
    vk::DescriptorSetLayout& _descriptorSetLayout;

    std::vector<glm::vec3> _linesData;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _frameData;
    std::array<GeometryPipeline::FrameData, MAX_FRAMES_IN_FLIGHT>& _geometryFrameData;
};