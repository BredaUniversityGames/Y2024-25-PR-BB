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
    IndirectCuller(const std::shared_ptr<GraphicsContext>& context, const CameraResource& camera, ResourceHandle<Buffer> targetBuffer, vk::DescriptorSet targetDescriptorSet, ResourceHandle<Buffer> visibilityBuffer, vk::DescriptorSet visibilityDescriptorSet);
    ~IndirectCuller();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene, bool isPrepass);

    NON_COPYABLE(IndirectCuller);
    NON_MOVABLE(IndirectCuller);

private:
    std::shared_ptr<GraphicsContext> _context;

    vk::PipelineLayout _cullingPipelineLayout;
    vk::Pipeline _cullingPipeline;

    const CameraResource& _camera;
    ResourceHandle<Buffer> _targetBuffer;
    vk::DescriptorSet _targetDescriptorSet;
    ResourceHandle<Buffer> _visibilityBuffer;
    vk::DescriptorSet _visibilityDescriptorSet;

    void CreateCullingPipeline();
};
