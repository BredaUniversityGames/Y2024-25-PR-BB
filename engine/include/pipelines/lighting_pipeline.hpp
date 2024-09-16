#pragma once

#include "include.hpp"
#include "gbuffers.hpp"
#include "mesh.hpp"
#include "swap_chain.hpp"

class LightingPipeline
{
public:
    LightingPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const ImageHandle hdrTarget, const CameraStructure& camera, const Cubemap& irradianceMap, const Cubemap& prefilterMap, const ImageHandle brdfLUT);
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
    const ImageHandle _hdrTarget;
    const CameraStructure& _camera;
    const Cubemap& _irradianceMap;
    const Cubemap& _prefilterMap;
    const ImageHandle _brdfLUT;

    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::DescriptorSet _descriptorSet;
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    vk::UniqueSampler _sampler;
};
