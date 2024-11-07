#pragma once

#include "bloom_settings.hpp"
#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "mesh.hpp"

#include <cstddef>
#include <memory>

class BloomSettings;
class VulkanContext;
struct RenderSceneDescription;

class SkydomePipeline final : public FrameGraphRenderPass
{
public:
    SkydomePipeline(const std::shared_ptr<VulkanContext>& context, ResourceHandle<Mesh> sphere, const CameraResource& camera, ResourceHandle<Image> hdrTarget,
        ResourceHandle<Image> brightnessTarget, ResourceHandle<Image> environmentMap, const GBuffers& _gBuffers, const BloomSettings& bloomSettings);
    ~SkydomePipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_COPYABLE(SkydomePipeline);
    NON_MOVABLE(SkydomePipeline);

private:
    struct PushConstants
    {
        uint32_t hdriIndex;
    } _pushConstants;

    std::shared_ptr<VulkanContext> _context;
    const CameraResource& _camera;
    ResourceHandle<Image> _hdrTarget;
    ResourceHandle<Image> _brightnessTarget;
    ResourceHandle<Image> _environmentMap;
    const GBuffers& _gBuffers;

    ResourceHandle<Mesh> _sphere;
    vk::UniqueSampler _sampler;
    const BloomSettings& _bloomSettings;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    void CreatePipeline();
};