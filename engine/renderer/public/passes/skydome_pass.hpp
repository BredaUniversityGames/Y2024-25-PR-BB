#pragma once

#include "bloom_settings.hpp"
#include "frame_graph.hpp"
#include "gbuffers.hpp"

#include <cstdint>
#include <memory>

class BloomSettings;
class GraphicsContext;
struct GPUMesh;
struct RenderSceneDescription;

class SkydomePass final : public FrameGraphRenderPass
{
public:
    SkydomePass(const std::shared_ptr<GraphicsContext>& context, ResourceHandle<GPUMesh> sphere, ResourceHandle<GPUImage> hdrTarget,
        ResourceHandle<GPUImage> brightnessTarget, ResourceHandle<GPUImage> environmentMap, const GBuffers& _gBuffers, const BloomSettings& bloomSettings);
    ~SkydomePass() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_COPYABLE(SkydomePass);
    NON_MOVABLE(SkydomePass);

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