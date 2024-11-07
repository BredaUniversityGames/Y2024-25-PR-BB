#pragma once

#include "common.hpp"
#include "vulkan_context.hpp"

struct Emitter;
class CameraResource;
class SwapChain;
class ECS;

class ParticlePipeline
{
public:
    ParticlePipeline(const VulkanContext& brain, const CameraResource& camera, const SwapChain& swapChain);
    ~ParticlePipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, std::shared_ptr<ECS> ecs, float deltaTime);

    NON_COPYABLE(ParticlePipeline);
    NON_MOVABLE(ParticlePipeline);

private:
    enum class ParticleBufferUsage
    {
        eParticle = 0,
        eAliveNew,
        eAliveCurrent,
        eDead,
        eCounter,
        eNone
    };
    enum class ShaderStages
    {
        eKickOff = 0,
        eEmit,
        eSimulate,
        eRenderInstanced,
        eNone
    };

    struct SimulatePushConstant
    {
        float deltaTime;
    } _simulatePushConstant;
    struct EmitPushConstant
    {
        uint32_t bufferOffset;
    } _emitPushConstant;

    const VulkanContext& _brain;
    const CameraResource& _camera;
    const SwapChain& _swapChain;

    std::vector<Emitter> _emitters;

    std::array<vk::Pipeline, 4> _pipelines;
    std::array<vk::PipelineLayout, 4> _pipelineLayouts;

    // particle instances storage buffers
    ResourceHandle<Buffer> _particleInstancesBuffer;
    ResourceHandle<Buffer> _culledIndicesBuffer;
    vk::DescriptorSet _instancesDescriptorSet;
    vk::DescriptorSetLayout _instancesDescriptorSetLayout;
    // particle storage buffers
    std::array<ResourceHandle<Buffer>, 5> _particlesBuffers;
    vk::DescriptorSet _particlesBuffersDescriptorSet;
    vk::DescriptorSetLayout _particlesBuffersDescriptorSetLayout;
    // emitter uniform buffers
    ResourceHandle<Buffer> _emittersBuffer;
    vk::DescriptorSet _emittersDescriptorSet;
    vk::DescriptorSetLayout _emittersBufferDescriptorSetLayout;
    // staging buffer
    vk::Buffer _stagingBuffer;
    VmaAllocation _stagingBufferAllocation;

    void UpdateEmitters(std::shared_ptr<ECS> ecs);

    void CreatePipelines();
    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();
    void CreateBuffers();

    void UpdateBuffers(vk::CommandBuffer commandBuffer);
    void UpdateParticleBuffersDescriptorSets();
    void UpdateParticleInstancesBuffersDescriptorSets();
    void UpdateEmittersBuffersDescriptorSets();
};