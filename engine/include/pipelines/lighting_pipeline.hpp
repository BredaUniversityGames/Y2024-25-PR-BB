#pragma once

#include "gbuffers.hpp"
#include "vulkan_brain.hpp"
#include "mesh.hpp"

class GPUScene;
class BloomSettings;
struct RenderSceneDescription;

class LightingPipeline
{
public:
    LightingPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, ResourceHandle<Image> hdrTarget, ResourceHandle<Image> brightnessTarget, const GPUScene& gpuScene, const CameraResource& camera, const BloomSettings& bloomSettings);
    ~LightingPipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene);

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
    const CameraResource& _camera;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    vk::UniqueSampler _sampler;
    vk::UniqueSampler _shadowSampler;

    const BloomSettings& _bloomSettings;
};
