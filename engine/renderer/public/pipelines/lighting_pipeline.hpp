#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"

#include <cstdint>
#include <memory>

class BloomSettings;
class GraphicsContext;
class CameraResource;

struct RenderSceneDescription;

class LightingPipeline final : public FrameGraphRenderPass
{
public:
    LightingPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, ResourceHandle<Image> hdrTarget, ResourceHandle<Image> brightnessTarget, const CameraResource& camera, const BloomSettings& bloomSettings);
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

    std::shared_ptr<GraphicsContext> _context;
    const GBuffers& _gBuffers;
    const ResourceHandle<Image> _hdrTarget;
    const ResourceHandle<Image> _brightnessTarget;
    const CameraResource& _camera;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    const BloomSettings& _bloomSettings;
};
