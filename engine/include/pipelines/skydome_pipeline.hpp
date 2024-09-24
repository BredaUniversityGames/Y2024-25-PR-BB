#pragma once

#include "swap_chain.hpp"
#include "mesh.hpp"

class SkydomePipeline
{
public:
    SkydomePipeline(const VulkanBrain& brain, MeshPrimitiveHandle&& sphere, const CameraStructure& camera, ResourceHandle<Image> hdrTarget, ResourceHandle<Image> environmentMap);
    ~SkydomePipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame);

    NON_COPYABLE(SkydomePipeline);
    NON_MOVABLE(SkydomePipeline);

private:
    struct PushConstants
    {
        uint32_t hdriIndex;
    } _pc;

    const VulkanBrain& _brain;
    const CameraStructure& _camera;
    ResourceHandle<Image> _hdrTarget;
    ResourceHandle<Image> _environmentMap;

    MeshPrimitiveHandle _sphere;
    vk::UniqueSampler _sampler;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    void CreatePipeline();
};