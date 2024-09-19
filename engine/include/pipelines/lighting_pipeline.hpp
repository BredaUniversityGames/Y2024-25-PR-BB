#pragma once

#include "gbuffers.hpp"
#include "mesh.hpp"
#include "swap_chain.hpp"

class LightingPipeline
{
public:
    LightingPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, ResourceHandle<Image> hdrTarget, const CameraStructure& camera, ResourceHandle<Image> irradianceMap, ResourceHandle<Image> prefilterMap, ResourceHandle<Image> brdfLUT);
    ~LightingPipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame);
    void UpdateGBufferViews();

    NON_MOVABLE(LightingPipeline);
    NON_COPYABLE(LightingPipeline);

private:
    void CreatePipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();

    const VulkanBrain& _brain;
    const GBuffers& _gBuffers;
    const ResourceHandle<Image> _hdrTarget;
    const CameraStructure& _camera;
    const ResourceHandle<Image> _irradianceMap;
    const ResourceHandle<Image> _prefilterMap;
    const ResourceHandle<Image> _brdfLUT;

    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::DescriptorSet _descriptorSet;
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    vk::UniqueSampler _sampler;
};
