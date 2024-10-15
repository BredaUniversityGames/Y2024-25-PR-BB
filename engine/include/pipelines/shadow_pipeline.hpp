﻿#pragma once

#include "gbuffers.hpp"
#include "mesh.hpp"
#include "geometry_pipeline.hpp"
#include "indirect_culler.hpp"

class BatchBuffer;
class RenderSceneDescription;

class ShadowPipeline
{
public:
    ShadowPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const GPUScene& gpuScene);
    ~ShadowPipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene);

    NON_MOVABLE(ShadowPipeline);
    NON_COPYABLE(ShadowPipeline);

private:
    void CreatePipeline(const GPUScene& gpuScene);
    void CreateDrawBufferDescriptorSet(const GPUScene& gpuScene);

    const VulkanBrain& _brain;
    const GBuffers& _gBuffers;

    CameraResource _shadowCamera;
    IndirectCuller _culler;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    ResourceHandle<Buffer> _drawBuffer;
    vk::DescriptorSet _drawBufferDescriptorSet;
};