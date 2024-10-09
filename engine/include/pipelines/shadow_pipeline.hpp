#pragma once

#include "gbuffers.hpp"
#include "mesh.hpp"
#include "geometry_pipeline.hpp"

class BatchBuffer;
class RenderSceneDescription;

class ShadowPipeline
{
public:
    ShadowPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const CameraStructure& camera, const GPUScene& gpuScene);
    ~ShadowPipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene, const BatchBuffer& batchBuffer);

    NON_MOVABLE(ShadowPipeline);
    NON_COPYABLE(ShadowPipeline);

private:
    void CreatePipeline(const GPUScene& gpuScene);

    const VulkanBrain& _brain;
    const GBuffers& _gBuffers;
    const CameraStructure& _camera;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;
};