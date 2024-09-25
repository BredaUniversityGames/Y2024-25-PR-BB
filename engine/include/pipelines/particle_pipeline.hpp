#pragma once
#include "camera.hpp"
#include "swap_chain.hpp"

class ParticlePipeline
{
public:
    ParticlePipeline(const VulkanBrain& brain, const SwapChain& swapChain, const CameraStructure& camera);
    ~ParticlePipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, uint32_t swapChainIndex);

    NON_COPYABLE(ParticlePipeline);
    NON_MOVABLE(ParticlePipeline);

private:
    struct UniformBuffer
    {
        vk::Buffer buffer;
        VmaAllocation bufferAllocation;
        void* bufferMapped;
    };

    const VulkanBrain& _brain;
    const SwapChain& _swapChain;
    const CameraStructure& _camera;

    // Particle buffers
    UniformBuffer _particleBuffer;
    UniformBuffer _aliveList[2];
    UniformBuffer _deadList;
    UniformBuffer _counterBuffer;
    // TODO: add more buffers (e.g. indirect buffers, constant, etc.)

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    void CreatePipeline();
    void CreateBuffers();
    void UpdateBuffers(); // might not be needed, but is here for now
};