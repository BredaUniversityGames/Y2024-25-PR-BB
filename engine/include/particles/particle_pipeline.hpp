#pragma once

struct CameraStructure;
struct Emitter;
class ECS;


class ParticlePipeline
{
public:
    ParticlePipeline(const VulkanBrain& brain, const CameraStructure& camera);
    ~ParticlePipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, ECS& ecs);

    NON_COPYABLE(ParticlePipeline);
    NON_MOVABLE(ParticlePipeline);

private:
    enum class SSBOUsage
    {
        eParticle = 0,
        eAliveNew,
        eAliveCurrent,
        eDead,
        eCounter,
        eNone
    };

    struct Buffer
    {
        vk::Buffer buffer;
        VmaAllocation bufferAllocation;
        void* bufferMapped;
        vk::DescriptorSet descriptorSet;
    };

    struct PushConstantSize
    {
        float padding;
    } _pushConstantSize;

    const VulkanBrain& _brain;
    const CameraStructure& _camera;

    std::vector<Emitter> _emitters;
    std::vector<std::string> _particlePaths;

    std::vector<vk::Pipeline> _pipelines;
    std::array<Buffer, 5> _storageBuffers;
    Buffer _emitterBuffer;

    vk::DescriptorSetLayout _storageLayout;
    vk::DescriptorSetLayout _uniformLayout;
    vk::PipelineLayout _pipelineLayout;

    void UpdateEmitters(ECS& ecs);

    void CreatePipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();
    void CreateBuffers();
    void UpdateBuffers();
    void UpdateParticleDescriptorSets();
};