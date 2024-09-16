#pragma once

#include "include.hpp"
#include "swap_chain.hpp"
#include "mesh.hpp"

class SkydomePipeline
{
public:
    SkydomePipeline(const VulkanBrain& brain, MeshPrimitiveHandle&& sphere, const CameraStructure& camera, const ImageHandle hdrTarget, const ImageHandle environmentMap);
    ~SkydomePipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame);

    NON_COPYABLE(SkydomePipeline);
    NON_MOVABLE(SkydomePipeline);


private:
    const VulkanBrain& _brain;
    const CameraStructure& _camera;
    const ImageHandle _hdrTarget;
    const ImageHandle _environmentMap;

    MeshPrimitiveHandle _sphere;
    vk::UniqueSampler _sampler;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;
    vk::DescriptorSet _descriptorSet;
    vk::DescriptorSetLayout _descriptorSetLayout;

    void CreatePipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSet();
};