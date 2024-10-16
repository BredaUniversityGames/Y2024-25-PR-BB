#pragma once

struct Emitter;
class CameraResource;
class ECS;

class ParticlePipeline
{
public:
    ParticlePipeline(const VulkanBrain& brain, const CameraResource& camera);
    ~ParticlePipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t _currentFrame, ECS& ecs, float deltaTime);

    NON_COPYABLE(ParticlePipeline);
    NON_MOVABLE(ParticlePipeline);

private:
    enum class SSBUsage
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

    struct FrameData
    {
        ResourceHandle<Buffer> buffer;
        vk::DescriptorSet descriptorSet;
    };

    const VulkanBrain& _brain;
    const CameraResource& _camera;

    std::vector<Emitter> _emitters;

    std::array<vk::Pipeline, 5> _pipelines;
    std::array<vk::PipelineLayout, 5> _pipelineLayouts;

    // particle instances storage buffer
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> _particleInstancesFrameData;
    vk::DescriptorSetLayout _particleInstancesDescriptorSetLayout;
    // particle storage buffers
    std::array<ResourceHandle<Buffer>, 5> _particlesBuffers;
    vk::DescriptorSet _particlesBuffersDescriptorSet;
    vk::DescriptorSetLayout _particlesBuffersDescriptorSetLayout;
    // emitter uniform buffer
    FrameData _emittersFrameData;
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