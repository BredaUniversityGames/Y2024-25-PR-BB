#pragma once

#include "gbuffers.hpp"
#include "mesh.hpp"

class GPUScene;
class BloomSettings;
struct RenderSceneDescription;

class LightingPipeline
{
public:
    LightingPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, ResourceHandle<Image> hdrTarget, ResourceHandle<Image> brightnessTarget, const GPUScene& gpuScene, const CameraStructure& camera, ResourceHandle<Image> irradianceMap, ResourceHandle<Image> prefilterMap, ResourceHandle<Image> brdfLUT, const BloomSettings& bloomSettings);
    ~LightingPipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene);
    void UpdateGBufferViews();

    NON_MOVABLE(LightingPipeline);
    NON_COPYABLE(LightingPipeline);

private:
    struct PushConstants
    {
        uint32_t albedoMIndex;
        uint32_t normalRIndex;
        uint32_t emissiveAOIndex;
        uint32_t positionIndex;
    } _pushConstants;

    void CreatePipeline(const GPUScene& gpuScene);

    const VulkanBrain& _brain;
    const GBuffers& _gBuffers;
    const ResourceHandle<Image> _hdrTarget;
    const ResourceHandle<Image> _brightnessTarget;
    const CameraStructure& _camera;
    const ResourceHandle<Image> _irradianceMap;
    const ResourceHandle<Image> _prefilterMap;
    const ResourceHandle<Image> _brdfLUT;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    vk::UniqueSampler _sampler;
    vk::UniqueSampler _shadowSampler;

    const BloomSettings& _bloomSettings;
};
