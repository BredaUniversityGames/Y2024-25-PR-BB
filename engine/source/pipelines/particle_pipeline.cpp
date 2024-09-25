#include "pipelines/particle_pipeline.hpp"

#include "vulkan_helper.hpp"
#include "../shaders/particles/particle_vars.glsl"

ParticlePipeline::ParticlePipeline(const VulkanBrain& brain, const SwapChain& swapChain, const CameraStructure& camera)
    : _brain(brain)
    , _swapChain(swapChain)
    , _camera(camera)
{
    CreatePipeline();
    CreateBuffers();
}

 ParticlePipeline::~ParticlePipeline()
{
    _brain.device.destroy(_pipeline);
    _brain.device.destroy(_pipelineLayout);
    vmaUnmapMemory(_brain.vmaAllocator, _particleBuffer.bufferAllocation);
    vmaDestroyBuffer(_brain.vmaAllocator, _particleBuffer.buffer, _particleBuffer.bufferAllocation);
    for(size_t i = 0; i < 2; i++)
    {
        vmaUnmapMemory(_brain.vmaAllocator, _aliveList[i].bufferAllocation);
        vmaDestroyBuffer(_brain.vmaAllocator, _aliveList[i].buffer, _aliveList[i].bufferAllocation);
    }
}

void ParticlePipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, uint32_t swapChainIndex)
{

}

void ParticlePipeline::CreatePipeline()
{

}

void ParticlePipeline::CreateBuffers()
{
    vk::DeviceSize particleBufferSize = sizeof(Particle) * MAX_PARTICLES;
    util::CreateBuffer(_brain, particleBufferSize,
        vk::BufferUsageFlagBits::eStorageBuffer,
        _particleBuffer.buffer, true, _particleBuffer.bufferAllocation,
        VMA_MEMORY_USAGE_GPU_ONLY,
        "Particle Storage buffer");
    util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, _particleBuffer.bufferAllocation, &_particleBuffer.bufferMapped), "Failed mapping memory for SSBO!");

    vk::DeviceSize indexBufferSize = sizeof(uint32_t) * MAX_PARTICLES;
    util::CreateBuffer(_brain, indexBufferSize,
        vk::BufferUsageFlagBits::eStorageBuffer,
        _deadList.buffer, true, _deadList.bufferAllocation,
        VMA_MEMORY_USAGE_GPU_ONLY,
        "Dead list buffer");
    util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, _deadList.bufferAllocation, &_deadList.bufferMapped), "Failed mapping memory for SSBO!");

    for (size_t i = 0; i < 2; i++)
    {
        util::CreateBuffer(_brain, indexBufferSize,
            vk::BufferUsageFlagBits::eStorageBuffer,
            _aliveList[i].buffer, true, _aliveList[i].bufferAllocation,
            VMA_MEMORY_USAGE_GPU_ONLY,
            "Alive list buffer");
        util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, _aliveList[i].bufferAllocation, &_aliveList[i].bufferMapped), "Failed mapping memory for SSBO!");
    }
}

void ParticlePipeline::UpdateBuffers()
{
    // TODO: check if needed and implement
}
