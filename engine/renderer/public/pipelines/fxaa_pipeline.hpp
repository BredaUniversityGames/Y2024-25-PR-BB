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

class FXAAPipeline final : public FrameGraphRenderPass
{
public:
    FXAAPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const ResourceHandle<GPUImage>& fxaaTarget, const ResourceHandle<GPUImage>& sourceTarget);
    ~FXAAPipeline() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    float& GetEdgeTreshholdMin() { return _pushConstants.edgeThresholdMin; }
    float& GetEdgeTreshholdMax() { return _pushConstants.edgeThresholdMax; }
    float& GetSubPixelQuality() { return _pushConstants.subPixelQuality; }
    int32_t& GetIterations() { return _pushConstants.iterations; }
    bool& GetEnableFXAA() { return _pushConstants.enableFXAA; }

    NON_MOVABLE(FXAAPipeline);
    NON_COPYABLE(FXAAPipeline);

private:
    struct PushConstants
    {
        uint32_t sourceIndex;
        uint32_t screenWidth;
        uint32_t screenHeight;
        int32_t iterations = 12;
        float edgeThresholdMin = 0.0312;
        float edgeThresholdMax = 0.125;
        float subPixelQuality = 0.75;
        bool enableFXAA = true;
    } _pushConstants;

    void CreatePipeline();
    void CreateBuffers();

    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();

    ResourceHandle<Buffer> _sampleKernelBuffer;
    const GBuffers& _gBuffers;
    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::DescriptorSet _descriptorSet;

    std::shared_ptr<GraphicsContext> _context;
    const ResourceHandle<GPUImage> _fxaaTarget;

    const ResourceHandle<GPUImage>& _source;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;
};
