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
    SSAOPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const ResourceHandle<GPUImage>& ssaoTarget);
    ~SSAOPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    float& GetAOStrength() { return _pushConstants.aoStrength; }
    float& GetAOBias() { return _pushConstants.aoBias; }
    float& GetAORadius() { return _pushConstants.aoRadius; }
    float& GetMinAODistance() { return _pushConstants.minAoDistance; }
    float& GetMaxAODistance() { return _pushConstants.maxAoDistance; }
    NON_MOVABLE(SSAOPipeline);
    NON_COPYABLE(SSAOPipeline);

private:
    struct PushConstants
    {
        uint32_t normalRIndex;
        uint32_t positionIndex;
        uint32_t ssaoNoiseIndex;
        uint32_t screenWidth = 1920; // just for refference
        uint32_t screenHeight = 1080;
        float aoStrength = 3.0f;
        float aoBias = 1.0;
        float aoRadius = 0.5f;
        float minAoDistance = 0.3f;
        float maxAoDistance = 1.0f;
    } _pushConstants;

    void CreatePipeline();
    void CreateBuffers();

    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();

    ResourceHandle<Buffer> _sampleKernelBuffer;

    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::DescriptorSet _descriptorSet;

    std::shared_ptr<GraphicsContext> _context;
    const GBuffers& _gBuffers;
    const ResourceHandle<GPUImage> _ssaoTarget;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    ResourceHandle<GPUImage> _ssaoNoise;
};
