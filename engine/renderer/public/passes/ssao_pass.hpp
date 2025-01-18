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

class SSAOPass final : public FrameGraphRenderPass
{
public:
    SSAOPass(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const ResourceHandle<GPUImage>& ssaoTarget);
    ~SSAOPass() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_MOVABLE(SSAOPass);
    NON_COPYABLE(SSAOPass);

private:
    struct PushConstants
    {
        uint32_t normalIndex;
        uint32_t positionIndex;
        uint32_t ssaoNoiseIndex;
        uint32_t ssaoRenderTargetWidth = 1920 / 2; // just for refference
        uint32_t ssaoRenderTargetHeight = 1080 / 2;
        float aoStrength = 2.0f;
        float aoBias = 0.01f;
        float aoRadius = 0.2f;
        float minAoDistance = 1.0f;
        float maxAoDistance = 3.0f;
    } _pushConstants;

    void CreatePipeline();
    void CreateBuffers();

    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();

    ResourceHandle<Buffer> _sampleKernelBuffer;
    ResourceHandle<Sampler> _noiseSampler;

    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::DescriptorSet _descriptorSet;

    std::shared_ptr<GraphicsContext> _context;
    const GBuffers& _gBuffers;
    const ResourceHandle<GPUImage> _ssaoTarget;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    ResourceHandle<GPUImage> _ssaoNoise;
};
