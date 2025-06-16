#pragma once

#include "common.hpp"
#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "resource_manager.hpp"

#include "vulkan_include.hpp"
#include <cstdint>
#include <memory>
#include <vk_mem_alloc.h>

class CameraResource;
class BloomSettings;
class ECSModule;
class GraphicsContext;

struct Buffer;
struct Emitter;
struct LocalEmitter;
struct RenderSceneDescription;

class ParticlePass final : public FrameGraphRenderPass
{
public:
    ParticlePass(const std::shared_ptr<GraphicsContext>& context, ECSModule& ecs, const GBuffers& gBuffers, const CameraBatch& cameraBatch, const ResourceHandle<GPUImage>& hdrTarget, const ResourceHandle<GPUImage>& brightnessTarget, const BloomSettings& bloomSettings);
    ~ParticlePass() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    void ResetParticles();

    NON_COPYABLE(ParticlePass);
    NON_MOVABLE(ParticlePass);

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
        eRenderIndexedIndirect,
        eNone
    };

    struct SimulatePushConstant
    {
        float deltaTime;
        float hzbSize;
        uint32_t localEmitterCount;
        uint32_t hzbIndex;
        uint32_t isReverseZ;
    } _simulatePushConstant;
    struct EmitPushConstant
    {
        uint32_t bufferOffset;
    } _emitPushConstant;

    std::shared_ptr<GraphicsContext> _context;
    ECSModule& _ecs;
    const GBuffers& _gBuffers;
    const CameraBatch& _cameraBatch;
    const ResourceHandle<GPUImage> _hdrTarget;
    const ResourceHandle<GPUImage> _brightnessTarget;
    const BloomSettings& _bloomSettings;

    std::vector<Emitter> _emitters;
    std::vector<LocalEmitter> _localEmitters;

    std::array<vk::Pipeline, 4> _pipelines;
    std::array<vk::PipelineLayout, 4> _pipelineLayouts;

    // indirect draw storage buffer
    ResourceHandle<Buffer> _drawCommandsBuffer;
    vk::DescriptorSet _drawCommandsDescriptorSet;
    vk::DescriptorSetLayout _drawCommandsDescriptorSetLayout;
    // particle instances storage buffer
    ResourceHandle<Buffer> _culledInstancesBuffer;
    vk::DescriptorSet _culledInstancesDescriptorSet;
    vk::DescriptorSetLayout _culledInstancesDescriptorSetLayout;
    // particle storage buffers
    std::array<ResourceHandle<Buffer>, 5> _particlesBuffers;
    vk::DescriptorSet _particlesBuffersDescriptorSet;
    vk::DescriptorSetLayout _particlesBuffersDescriptorSetLayout;
    // emitter uniform buffers
    ResourceHandle<Buffer> _emittersBuffer;
    vk::DescriptorSet _emittersDescriptorSet;
    vk::DescriptorSetLayout _emittersBufferDescriptorSetLayout;
    // emitter staging buffer
    std::array<vk::Buffer, MAX_FRAMES_IN_FLIGHT> _emitterStagingBuffer;
    std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> _emitterStagingBufferAllocation;
    // local emitter uniform buffer
    ResourceHandle<Buffer> _localEmittersBuffer;
    vk::DescriptorSet _localEmittersDescriptorSet;
    vk::DescriptorSetLayout _localEmittersDescriptorSetLayout;
    // local emitter staging buffer
    std::array<vk::Buffer, MAX_FRAMES_IN_FLIGHT> _localEmitterStagingBuffer;
    std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> _localEmitterStagingBufferAllocation;
    // buffers for rendering
    ResourceHandle<Buffer> _vertexBuffer;
    ResourceHandle<Buffer> _indexBuffer;

    void RecordKickOff(vk::CommandBuffer commandBuffer);
    void RecordEmit(vk::CommandBuffer commandBuffer);
    void RecordSimulate(vk::CommandBuffer commandBuffer, const CameraResource& camera, float deltaTime, uint32_t currentFrame);
    void RecordRenderIndexedIndirect(vk::CommandBuffer commandBuffer, const RenderSceneDescription& scene, uint32_t currentFrame);

    void UpdateEmitters(vk::CommandBuffer commandBuffer, uint32_t currentFrame);

    void CreatePipelines();
    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();
    void CreateBuffers();

    void UpdateAliveLists();
    void UpdateParticleBuffersDescriptorSets();
    void UpdateParticleInstancesBufferDescriptorSet();
    void UpdateEmittersBuffersDescriptorSets();
    void UpdateLocalEmittersBuffersDescriptorSets();
    void UpdateDrawCommandsBufferDescriptorSet();
};