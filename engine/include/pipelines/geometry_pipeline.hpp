#pragma once

#include "gbuffers.hpp"
#include "mesh.hpp"

class BatchBuffer;

struct alignas(16) InstanceData
{
    glm::mat4 model;
    uint32_t materialIndex;
};

class GeometryPipeline
{
public:
    struct FrameData
    {
        vk::Buffer storageBuffer;
        VmaAllocation storageBufferAllocation;
        void* storageBufferMapped;
        vk::DescriptorSet descriptorSet;
    };

    GeometryPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const CameraStructure& camera);

    ~GeometryPipeline();

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT>& GetFrameData() { return _frameData; }
    vk::DescriptorSetLayout& DescriptorSetLayout() { return _descriptorSetLayout; }

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const SceneDescription& scene, const BatchBuffer& batchBuffer);

    NON_MOVABLE(GeometryPipeline);
    NON_COPYABLE(GeometryPipeline);

private:
    void CreatePipeline();

    void CreateDescriptorSetLayout();

    void CreateDescriptorSets();

    void CreateInstanceBuffers();

    void UpdateGeometryDescriptorSet(uint32_t frameIndex);

    void UpdateInstanceData(uint32_t currentFrame, const SceneDescription& scene);

    const VulkanBrain& _brain;
    const GBuffers& _gBuffers;
    const CameraStructure& _camera;

    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _frameData;
};
