#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "settings.hpp"
#include "swap_chain.hpp"

#include <cstdint>
#include <memory>

class BloomSettings;
class GraphicsContext;

class VolumetricPass final : public FrameGraphRenderPass
{
public:
    VolumetricPass(const std::shared_ptr<GraphicsContext>& context, const Settings::Tonemapping& settings, ResourceHandle<GPUImage> hdrTarget, ResourceHandle<GPUImage> bloomTarget, ResourceHandle<GPUImage> outputTarget, const SwapChain& _swapChain, const GBuffers& gBuffers, const BloomSettings& bloomSettings);
    ~VolumetricPass() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    void SetShotRayOrigin(const glm::vec3& origin) { _pushConstants.rayOrigin = glm::vec4(origin, 0.3f); }
    void SetShotRayDirection(const glm::vec3& direction) { _pushConstants.rayDirection = glm::vec4(direction, 0.3f); }

    NON_COPYABLE(VolumetricPass);
    NON_MOVABLE(VolumetricPass);

private:


    struct PushConstants
    {
        // Register 0 (16 bytes)
        uint32_t hdrTargetIndex;
        uint32_t bloomTargetIndex;
        uint32_t depthIndex;
        uint32_t normalRIndex;

        uint32_t screenWidth;
        uint32_t screenHeight;

        glm::vec4 rayOrigin;
        glm::vec4 rayDirection;
    } _pushConstants;

    std::shared_ptr<GraphicsContext> _context;
    const Settings::Tonemapping& _settings;
    const SwapChain& _swapChain;
    const GBuffers& _gBuffers;
    ResourceHandle<GPUImage> _hdrTarget;
    ResourceHandle<GPUImage> _bloomTarget;
    ResourceHandle<GPUImage> _outputTarget;

    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;

    const BloomSettings& _bloomSettings;
    float timePassed = 0.0f;

    void CreatePipeline();
    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();
};