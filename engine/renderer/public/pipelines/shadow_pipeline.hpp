#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "indirect_culler.hpp"
#include "mesh.hpp"

#include <cstddef>
#include <memory>

class BatchBuffer;
class VulkanContext;

struct RenderSceneDescription;

class ShadowPipeline final : public FrameGraphRenderPass
{
public:
    ShadowPipeline(const std::shared_ptr<VulkanContext>& context, const GBuffers& gBuffers, const GPUScene& gpuScene);
    ~ShadowPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(ShadowPipeline);
    NON_COPYABLE(ShadowPipeline);

private:
    void CreatePipeline();
    void CreateDrawBufferDescriptorSet(const GPUScene& gpuScene);

    std::shared_ptr<VulkanContext> _context;
    const GBuffers& _gBuffers;

    CameraResource _shadowCamera;
    IndirectCuller _culler;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    ResourceHandle<Buffer> _drawBuffer;
    vk::DescriptorSet _drawBufferDescriptorSet;
};