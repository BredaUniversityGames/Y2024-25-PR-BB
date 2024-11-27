#pragma once

#include "gbuffers.hpp"
#include "common.hpp"
#include "vulkan_brain.hpp"

struct Emitter;
class CameraResource;
class RenderSceneDescription;
class SwapChain;
class ECS;

class ParticlePipeline
{
public:
    ParticlePipeline(const VulkanBrain& brain, const GBuffers& gBuffers, ResourceHandle<Image> hdrTarget, const CameraResource& camera, const SwapChain& swapChain);
    ~ParticlePipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene, ECS& ecs, float deltaTime);

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
    struct RenderPushConstant
    {
        glm::vec3 cameraRight;
        float _rightPadding;
        glm::vec3 cameraUp;
        float _upPadding;
    } _renderPushConstant;

    const VulkanBrain& _brain;
    const GBuffers& _gBuffers;
    const ResourceHandle<Image> _hdrTarget;
    const CameraResource& _camera;
    const SwapChain& _swapChain;

    std::vector<Emitter> _emitters;

    std::array<vk::Pipeline, 4> _pipelines;
    std::array<vk::PipelineLayout, 4> _pipelineLayouts;

    // particle instances storage buffers
    ResourceHandle<Buffer> _culledInstancesBuffer;
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
    // buffers for rendering
    ResourceHandle<Buffer> _vertexBuffer;
    ResourceHandle<Buffer> _indexBuffer;

    void RecordKickOff(vk::CommandBuffer commandBuffer);
    void RecordEmit(vk::CommandBuffer commandBuffer);
    void RecordSimulate(vk::CommandBuffer commandBuffer, float deltaTime);
    void RecordRenderIndexed(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene);

    void UpdateEmitters(ECS& ecs, vk::CommandBuffer commandBuffer);

    void CreatePipelines();
    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();
    void CreateBuffers();

    void UpdateAliveLists();
    void UpdateParticleBuffersDescriptorSets();
    void UpdateParticleInstancesBufferDescriptorSet();
    void UpdateEmittersBuffersDescriptorSets();
};