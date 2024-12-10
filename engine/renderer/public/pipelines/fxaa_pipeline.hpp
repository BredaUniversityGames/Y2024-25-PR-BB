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

class FXAAPipeline final : public FrameGraphRenderPass
{
public:
    FXAAPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const ResourceHandle<GPUImage>& fxaaTarget, const ResourceHandle<GPUImage>& sourceTarget);
    ~FXAAPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(FXAAPipeline);
    NON_COPYABLE(FXAAPipeline);

private:
    struct PushConstants
    {
        uint32_t sourceIndex;
        uint32_t screenWidth;
        uint32_t screenHeight;
    } _pushConstants;

    void CreatePipeline();
    void CreateBuffers();

    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();

    ResourceHandle<Buffer> _sampleKernelBuffer;
    const GBuffers& _gBuffers;
    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::DescriptorSet _descriptorSet;

    std::shared_ptr<GraphicsContext> _context;
    const ResourceHandle<GPUImage> _fxaaTarget;

    const ResourceHandle<GPUImage>& _source;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;
};
