#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"

#include <cstdint>
#include <memory>

class BloomSettings;
class GraphicsContext;
class CameraResource;
struct GPUImage;
struct RenderSceneDescription;

class LightingPipeline final : public FrameGraphRenderPass
{
public:
    LightingPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const ResourceHandle<GPUImage>& hdrTarget, const ResourceHandle<GPUImage>& brightnessTarget, const BloomSettings& bloomSettings, const ResourceHandle<GPUImage>& ssaoTarget);
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
        uint32_t ssaoIndex;
        uint32_t depthIndex;
    } _pushConstants;

    void CreatePipeline();

    std::shared_ptr<GraphicsContext> _context;
    const GBuffers& _gBuffers;
    const ResourceHandle<GPUImage> _hdrTarget;
    const ResourceHandle<GPUImage> _brightnessTarget;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    const BloomSettings& _bloomSettings;
    const ResourceHandle<GPUImage> _ssaoTarget;
};
