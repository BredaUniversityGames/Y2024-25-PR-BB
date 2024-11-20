#pragma once

#include "common.hpp"
#include "gpu_resources.hpp"

#include <cstddef>
#include <memory>

class GPUScene;
class GraphicsContext;
class CameraResource;
struct RenderSceneDescription;

class IndirectCuller // TODO: Convert this to FrameGraphRenderPass
{
public:
    IndirectCuller(const std::shared_ptr<GraphicsContext>& context, const GPUScene& gpuScene);
    ~IndirectCuller();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene, const CameraResource& camera, ResourceHandle<Buffer> targetBuffer, vk::DescriptorSet targetDescriptorSet);

    NON_COPYABLE(IndirectCuller);
    NON_MOVABLE(IndirectCuller);

private:
    std::shared_ptr<GraphicsContext> _context;

    vk::PipelineLayout _cullingPipelineLayout;
    vk::Pipeline _cullingPipeline;

    void CreateCullingPipeline(const GPUScene& gpuScene);
};
