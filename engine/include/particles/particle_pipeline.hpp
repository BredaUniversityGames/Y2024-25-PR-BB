#pragma once

struct Emitter;
class CameraResource;
class ECS;

class ParticlePipeline
{
public:
    ParticlePipeline(const VulkanBrain& brain, const CameraResource& camera);
    ~ParticlePipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, ECS& ecs);

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

    std::vector<Emitter> _emitters;

    std::vector<std::string> _shaderPaths;
    std::vector<vk::Pipeline> _pipelines;
    std::vector<vk::PipelineLayout> _pipelineLayouts;
    // ssbs
    std::array<ResourceHandle<Buffer>, 5> _storageBuffers;
    vk::DescriptorSet _storageBufferDescriptorSet;
    vk::DescriptorSetLayout _storageLayout;
    // ub
    ResourceHandle<Buffer> _emitterBuffer;
    vk::DescriptorSet _emitterBufferDescriptorSet;
    vk::DescriptorSetLayout _uniformLayout;

    void UpdateEmitters(ECS& ecs);

    void CreatePipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();
    void CreateBuffers();
    void UpdateBuffers();
    void UpdateParticleDescriptorSets();
};