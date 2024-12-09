#pragma once

#include "bloom_settings.hpp"
#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "vertex.hpp"

#include <cstddef>
#include <memory>

class BloomSettings;
class GraphicsContext;
struct RenderSceneDescription;

class SkydomePipeline final : public FrameGraphRenderPass
{
public:
    SkydomePipeline(const std::shared_ptr<GraphicsContext>& context, ResourceHandle<GPUMesh> sphere, ResourceHandle<GPUImage> hdrTarget,
        ResourceHandle<GPUImage> brightnessTarget, ResourceHandle<GPUImage> environmentMap, const GBuffers& _gBuffers, const BloomSettings& bloomSettings);
    ~SkydomePipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_COPYABLE(SkydomePipeline);
    NON_MOVABLE(SkydomePipeline);

private:
    struct PushConstants
    {
        uint32_t hdriIndex;
    } _pushConstants;

    std::shared_ptr<GraphicsContext> _context;
    ResourceHandle<GPUImage> _hdrTarget;
    ResourceHandle<GPUImage> _brightnessTarget;
    ResourceHandle<GPUImage> _environmentMap;
    const GBuffers& _gBuffers;

    ResourceHandle<GPUMesh> _sphere;
    const BloomSettings& _bloomSettings;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    void CreatePipeline();
};