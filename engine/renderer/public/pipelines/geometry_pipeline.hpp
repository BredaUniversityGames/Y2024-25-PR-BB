#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "indirect_culler.hpp"

#include <memory>

class BatchBuffer;
class GPUScene;
class VulkanContext;

struct RenderSceneDescription;

class GeometryPipeline final : public FrameGraphRenderPass
{
public:
    GeometryPipeline(const std::shared_ptr<VulkanContext>& context, const GBuffers& gBuffers, const CameraResource& camera, const GPUScene& gpuScene);
    ~GeometryPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(GeometryPipeline);
    NON_COPYABLE(GeometryPipeline);

private:
    void CreatePipeline();
    void CreateDrawBufferDescriptorSet(const GPUScene& gpuScene);

    std::shared_ptr<VulkanContext> _context;
    const GBuffers& _gBuffers;
    const CameraResource& _camera;

    IndirectCuller _culler;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    ResourceHandle<Buffer> _drawBuffer;
    vk::DescriptorSet _drawBufferDescriptorSet;
};
