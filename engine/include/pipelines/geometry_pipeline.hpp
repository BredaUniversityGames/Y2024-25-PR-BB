#pragma once

#include "gbuffers.hpp"
#include "mesh.hpp"

struct UBO
{
    alignas(16) glm::mat4 model;
};

constexpr uint32_t MAX_MESHES = 128;

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

    GeometryPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, vk::DescriptorSetLayout materialDescriptorSetLayout, const CameraStructure& camera);
    ~GeometryPipeline();

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT>& GetFrameData() { return _frameData; }

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const SceneDescription& scene);

    NON_MOVABLE(GeometryPipeline);
    NON_COPYABLE(GeometryPipeline);

private:


    void CreatePipeline(vk::DescriptorSetLayout materialDescriptorSetLayout);
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();
    void CreateUniformBuffers();
    void UpdateGeometryDescriptorSet(uint32_t frameIndex);
    void UpdateUniformData(uint32_t currentFrame, const std::vector<glm::mat4> transforms, const Camera& camera);

    const VulkanBrain& _brain;
    const GBuffers& _gBuffers;
    const CameraStructure& _camera;

    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _frameData;
};
