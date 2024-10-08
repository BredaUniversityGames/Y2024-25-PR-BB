#pragma once

#include "gbuffers.hpp"
#include "mesh.hpp"

class BatchBuffer;
class GPUScene;

class GeometryPipeline
{
public:
    GeometryPipeline(const VulkanBrain& brain, const GBuffers& gBuffers, const CameraStructure& camera, const GPUScene& gpuScene);

    ~GeometryPipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const SceneDescription& scene, const GPUScene& gpuScene, const BatchBuffer& batchBuffer);

    NON_MOVABLE(GeometryPipeline);
    NON_COPYABLE(GeometryPipeline);

private:
    void CreatePipeline(const GPUScene& gpuScene);

    const VulkanBrain& _brain;
    const GBuffers& _gBuffers;
    const CameraStructure& _camera;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    std::vector<vk::DrawIndexedIndirectCommand> _drawCommands;
};
