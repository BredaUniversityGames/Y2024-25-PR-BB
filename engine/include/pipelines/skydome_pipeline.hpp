#pragma once

#include "swap_chain.hpp"
#include "mesh.hpp"

struct HDRTarget;

class SkydomePipeline
{
public:
    SkydomePipeline(const VulkanBrain& brain, MeshPrimitiveHandle&& sphere, const CameraStructure& camera, const HDRTarget& hdrTarget, const TextureHandle& environmentMap);
    ~SkydomePipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame);

    NON_COPYABLE(SkydomePipeline);
    NON_MOVABLE(SkydomePipeline);


private:
    const VulkanBrain& _brain;
    const CameraStructure& _camera;
    const HDRTarget& _hdrTarget;
    const TextureHandle& _environmentMap;

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