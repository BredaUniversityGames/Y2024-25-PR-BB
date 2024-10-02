#pragma once

#include "gbuffers.hpp"
#include "mesh.hpp"

struct alignas(64) UBO
{
    glm::mat4 model;
    uint32_t materialIndex;
};

constexpr uint32_t MAX_MESHES = 2048;

class GeometryPipeline
{
public:
    struct FrameData
    {
        vk::Buffer uniformBuffer;
        VmaAllocation uniformBufferAllocation;
        void* uniformBufferMapped;
        vk::DescriptorSet descriptorSet;
    };

    GeometryPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const CameraStructure& camera);

    ~GeometryPipeline();

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT>& GetFrameData() { return _frameData; }
    vk::DescriptorSetLayout& DescriptorSetLayout() { return _descriptorSetLayout; }

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const SceneDescription& scene);

    NON_MOVABLE(GeometryPipeline);
    NON_COPYABLE(GeometryPipeline);

private:
    void CreatePipeline();

    void CreateDescriptorSetLayout();

    void CreateDescriptorSets();

    void CreateUniformBuffers();

    void UpdateGeometryDescriptorSet(uint32_t frameIndex);

    void UpdateUniformData(uint32_t currentFrame, const SceneDescription& scene);

    const VulkanBrain& _brain;
    const GBuffers& _gBuffers;
    const CameraStructure& _camera;

    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _frameData;
};
