#pragma once

#include "gbuffers.hpp"
#include "mesh.hpp"
#include "geometry_pipeline.hpp"
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

    PhysicsRenderPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const CameraStructure& camera, GeometryPipeline& geometryPipeline);
    ~PhysicsRenderPipeline();

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT>& GetFrameData() { return _frameData; }

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame);

    NON_MOVABLE(PhysicsRenderPipeline);
    NON_COPYABLE(PhysicsRenderPipeline);

    std::vector<glm::vec3> linePoints;

private:
    void CreatePipeline();

    void CreateVertexBuffer();

    void UpdateVertexData(uint32_t currentFrame);

    const VulkanBrain& _brain;
    const GBuffers& _gBuffers;
    const CameraStructure& _camera;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;
    vk::DescriptorSetLayout& _descriptorSetLayout;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _frameData;
    std::array<GeometryPipeline::FrameData, MAX_FRAMES_IN_FLIGHT>& _geometryFrameData;
};