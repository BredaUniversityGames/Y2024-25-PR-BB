#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "indirect_culler.hpp"
#include "mesh.hpp"
#include "vulkan_brain.hpp"

class BatchBuffer;
class GPUScene;
struct RenderSceneDescription;

class GeometryPipeline final : public FrameGraphRenderPass
{
public:
    GeometryPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const CameraResource& camera, const GPUScene& gpuScene);
    ~GeometryPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(GeometryPipeline);
    NON_COPYABLE(GeometryPipeline);

private:
    void CreatePipeline();
    void CreateDrawBufferDescriptorSet(const GPUScene& gpuScene);

    const VulkanBrain& _brain;
    const GBuffers& _gBuffers;
    const CameraResource& _camera;

    IndirectCuller _culler;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    ResourceHandle<Buffer> _drawBuffer;
    vk::DescriptorSet _drawBufferDescriptorSet;
};
