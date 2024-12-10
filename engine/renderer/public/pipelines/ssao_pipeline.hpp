#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"

#include <cstdint>
#include <memory>

class BloomSettings;
class GraphicsContext;
class CameraResource;
struct GPUImage;
struct RenderSceneDescription;

class SSAOPipeline final : public FrameGraphRenderPass
{
public:
    SSAOPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const ResourceHandle<GPUImage>& ssaoTarget, const CameraResource& camera);
    ~SSAOPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    float& GetAOStrength() { return _pushConstants.aoStrength; }
    float& GetAOBias() { return _pushConstants.aoBias; }
    float& GetAORadius() { return _pushConstants.aoRadius; }
    NON_MOVABLE(SSAOPipeline);
    NON_COPYABLE(SSAOPipeline);

private:
    struct PushConstants
    {
        uint32_t normalRIndex;
        uint32_t positionIndex;
        uint32_t ssaoNoiseIndex;
        float aoStrength = 1.0f;
        float aoBias = 0.025f;
        float aoRadius = 0.5f;
    } _pushConstants;

    void CreatePipeline();
    void CreateBuffers();

    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();

    ResourceHandle<Buffer> _sampleKernelBuffer;
    ResourceHandle<Buffer> _noiseBuffer;

    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::DescriptorSet _descriptorSet;

    std::shared_ptr<GraphicsContext> _context;
    const GBuffers& _gBuffers;
    const ResourceHandle<GPUImage> _ssaoTarget;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;
    const CameraResource& _camera;

    ResourceHandle<GPUImage> _ssaoNoise;
};
