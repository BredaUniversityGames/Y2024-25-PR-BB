#pragma once

#include "bloom_settings.hpp"
#include "vulkan_brain.hpp"
#include "mesh.hpp"
#include "frame_graph.hpp"

class BloomSettings;
class RenderSceneDescription;

class SkydomePipeline : public FrameGraphRenderPass
{
public:
    SkydomePipeline(const VulkanBrain& brain, ResourceHandle<Mesh> sphere, const CameraResource& camera, ResourceHandle<Image> hdrTarget,
        ResourceHandle<Image> brightnessTarget, ResourceHandle<Image> environmentMap, const BloomSettings& bloomSettings);
    ~SkydomePipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_COPYABLE(SkydomePipeline);
    NON_MOVABLE(SkydomePipeline);

private:
    struct PushConstants
    {
        uint32_t hdriIndex;
    } _pushConstants;

    const VulkanBrain& _brain;
    const CameraResource& _camera;
    ResourceHandle<Image> _hdrTarget;
    ResourceHandle<Image> _brightnessTarget;
    ResourceHandle<Image> _environmentMap;

    ResourceHandle<Mesh> _sphere;
    vk::UniqueSampler _sampler;
    const BloomSettings& _bloomSettings;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    void CreatePipeline();
};