#pragma once
#include "camera.hpp"
#include "swap_chain.hpp"

class ParticlePipeline
{
public:
    ParticlePipeline(const VulkanBrain& brain, const CameraStructure& camera);
    ~ParticlePipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, float deltaTime);

    enum class EmitterType
    {
        BILLBOARD,
        RIBBON
    };

    // TODO: expand upon options (color/texture, size, mass, etc)
    void SpawnEmitter(glm::vec3 position, uint32_t count, EmitterType type = EmitterType::BILLBOARD);

    NON_COPYABLE(ParticlePipeline);
    NON_MOVABLE(ParticlePipeline);

private:
    // Buffer related structs
    enum class SSBOUsage
    {
        PARTICLE = 0,
        ALIVE_NEW,
        ALIVE_CURRENT,
        DEAD,
        COUNTER
    };
    struct Buffer
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

    // Structs in line with shaders
    struct Emitter
    {
        glm::vec3 position = { 0.0f, 0.0f, 0.0f };
        uint32_t count = 0;
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

    // temporary values for testing/progress
    const uint32_t MAX_EMITTERS = 64;
    const uint32_t MAX_PARTICLES = 1024;
    const uint32_t MAX_PARTICLE_COUNTERS = 3;

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

    void CreatePipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();
    void CreateBuffers();
    void UpdateBuffers();
    void UpdateParticleDescriptorSets();
};