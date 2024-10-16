#pragma once

struct Emitter;
class CameraResource;
class SwapChain;
class ECS;

class ParticlePipeline
{
public:
    ParticlePipeline(const VulkanBrain& brain, const CameraResource& camera, const SwapChain& swapChain);
    ~ParticlePipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t _currentFrame, ECS& ecs, float deltaTime);

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

    // push constants
    struct SimulatePushConstant
    {
        float deltaTime;
    } _simulatePushConstant;
    struct EmitPushConstant
    {
        uint32_t bufferOffset;
    } _emitPushConstant;

    const VulkanBrain& _brain;
    const CameraResource& _camera;
    const SwapChain& _swapChain;

    std::vector<Emitter> _emitters;

    std::array<vk::Pipeline, 4> _pipelines;
    std::array<vk::PipelineLayout, 4> _pipelineLayouts;

    // particle instances storage buffers
    std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT> _particleInstancesBuffers;
    std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT> _culledIndicesBuffers;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _instancesDescriptorSets;
    vk::DescriptorSetLayout _instancesDescriptorSetLayout;
    // particle storage buffers
    std::array<ResourceHandle<Buffer>, 5> _particlesBuffers;
    vk::DescriptorSet _particlesBuffersDescriptorSet;
    vk::DescriptorSetLayout _particlesBuffersDescriptorSetLayout;
    // emitter uniform buffer
    ResourceHandle<Buffer> _emittersBuffer;
    vk::DescriptorSet _emittersDescriptorSet;
    vk::DescriptorSetLayout _emittersBufferDescriptorSetLayout;

    void UpdateEmitters(ECS& ecs);

    void CreatePipelines();
    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();
    void CreateBuffers();

    void UpdateBuffers();
    void UpdateParticleBuffersDescriptorSets();
    void UpdateParticleInstancesData(uint32_t frameIndex);
};