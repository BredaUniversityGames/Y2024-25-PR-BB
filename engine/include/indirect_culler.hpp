#pragma once

#include "common.hpp"
#include "gpu_resources.hpp"

class GPUScene;
class VulkanBrain;
class CameraResource;
struct RenderSceneDescription;

class IndirectCuller // TODO: Convert this to FrameGraphRenderPass
{
public:
    IndirectCuller(const VulkanBrain& brain, const GPUScene& gpuScene);
    ~IndirectCuller();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene, const CameraResource& camera, ResourceHandle<Buffer> targetBuffer, vk::DescriptorSet targetDescriptorSet);

    NON_COPYABLE(IndirectCuller);
    NON_MOVABLE(IndirectCuller);

private:
    const VulkanBrain& _brain;

    vk::PipelineLayout _cullingPipelineLayout;
    vk::Pipeline _cullingPipeline;

    void CreateCullingPipeline(const GPUScene& gpuScene);
};
