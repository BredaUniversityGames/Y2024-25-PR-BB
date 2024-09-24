#pragma once

#include "include.hpp"
#include "gbuffers.hpp"
#include "mesh.hpp"
#include "geometry_pipeline.hpp"
class ShadowPipeline
{
public:
    ShadowPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const CameraStructure& camera, GeometryPipeline& geometryPipeline);
    ~ShadowPipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const SceneDescription& scene);

    NON_MOVABLE(ShadowPipeline);
    NON_COPYABLE(ShadowPipeline);

private:

    void CreatePipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();

    void CreateUniformBuffers();
     void UpdateUniformData(uint32_t currentFrame, const std::vector<glm::mat4>& transforms);


    const VulkanBrain& _brain;
    const GBuffers& _gBuffers;
    const CameraStructure& _camera;
    //GeometryPipeline& _geometryPipeline;

    vk::DescriptorSetLayout _descriptorSetLayout; //remove / getter
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    const std::array<GeometryPipeline::FrameData, MAX_FRAMES_IN_FLIGHT>& _frameData; // getter

};