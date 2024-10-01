#pragma once
#include "camera.hpp"
#include "swap_chain.hpp"

class ParticlePipeline
{
public:
    ParticlePipeline(const VulkanBrain& brain, const CameraStructure& camera);
    ~ParticlePipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, float deltaTime);

    NON_COPYABLE(ParticlePipeline);
    NON_MOVABLE(ParticlePipeline);

private:
    enum class SSBOUsage
    {
        PARTICLE = 0,
        ALIVE_NEW,
        ALIVE_CURRENT,
        DEAD,
        COUNTER
    };

    struct Particle
    {
        glm::vec3 position = { 0.0f, 0.0f, 0.0f };
        float mass = 0.0f;
        glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };
        float maxLife = 5.0f;
        float life = 5.0f;
        glm::vec3 padding = { 0.0f, 0.0f, 0.0f };
    };

    struct StorageBuffer
    {
        vk::Buffer buffer;
        VmaAllocation bufferAllocation;
        void* bufferMapped;
        vk::DescriptorSet descriptorSet;
    };

    struct PushConstants
    {
        float deltaTime;
    } _pushConstants;

    const uint32_t MAX_PARTICLES = 1024; // temporary value
    const uint32_t MAX_PARTICLE_COUNTERS = 3;

    const VulkanBrain& _brain;
    const CameraStructure& _camera;

    std::array<StorageBuffer, 5> _storageBuffers;

    std::vector<std::string> _particlePaths;
    std::vector<vk::Pipeline> _pipelines;

    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::PipelineLayout _pipelineLayout;

    void CreatePipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();
    void CreateBuffers();
    void UpdateBuffers(); // might not be needed, but is here for now
    void UpdateParticleDescriptorSets();
};