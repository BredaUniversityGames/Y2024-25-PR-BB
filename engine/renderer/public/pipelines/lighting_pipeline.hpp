#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "mesh.hpp"
#include "vulkan_brain.hpp"

class GPUScene;
class BloomSettings;
struct RenderSceneDescription;

class LightingPipeline final : public FrameGraphRenderPass
{
public:
    LightingPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, ResourceHandle<Image> hdrTarget, ResourceHandle<Image> brightnessTarget, const CameraResource& camera, const BloomSettings& bloomSettings);
    ~LightingPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

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

    void CreatePipeline();

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
