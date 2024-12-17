#pragma once

#include "common.hpp"
#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "resource_manager.hpp"

#include <cstdint>
#include <memory>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

struct Emitter;
class CameraResource;
class BloomSettings;
struct RenderSceneDescription;
class ECSModule;
class GraphicsContext;
struct Buffer;

class ParticlePipeline final : public FrameGraphRenderPass
{
public:
    ParticlePipeline(const std::shared_ptr<GraphicsContext>& context, ECSModule& ecs, const GBuffers& gBuffers, const ResourceHandle<GPUImage>& hdrTarget, const ResourceHandle<GPUImage>& brightnessTarget, const BloomSettings& bloomSettings);
    ~ParticlePipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

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

    std::shared_ptr<GraphicsContext> _context;
    ECSModule& _ecs;
    const GBuffers& _gBuffers;
    const ResourceHandle<GPUImage> _hdrTarget;
    const ResourceHandle<GPUImage> _brightnessTarget;
    const BloomSettings& _bloomSettings;

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
    void RecordRenderIndexed(vk::CommandBuffer commandBuffer, const RenderSceneDescription& scene, uint32_t currentFrame);

    void UpdateEmitters(vk::CommandBuffer commandBuffer);

    void CreatePipelines();
    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();
    void CreateBuffers();

    void UpdateAliveLists();
    void UpdateParticleBuffersDescriptorSets();
    void UpdateParticleInstancesBufferDescriptorSet();
    void UpdateEmittersBuffersDescriptorSets();
};