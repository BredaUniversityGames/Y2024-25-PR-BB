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

    void AddGunShot(const glm::vec3 origin, const glm::vec3 direction)
    {
        // Place the new GunShot at the current index
        _pushConstants.gunShots[next_gunshot_index].origin = glm::vec4(origin, 0.3);
        _pushConstants.gunShots[next_gunshot_index].direction = glm::vec4(direction, 0.3);

        // Increment the index and wrap it around if it reaches MAX_GUNSHOTS
        next_gunshot_index = (next_gunshot_index + 1) % MAX_GUN_SHOTS;
    }

    NON_COPYABLE(VolumetricPass);
    NON_MOVABLE(VolumetricPass);

private:
    struct GunShot
    {
        glm::vec4 origin;
        glm::vec4 direction;
    };

    struct PushConstants
    {
        uint32_t hdrTargetIndex;
        uint32_t bloomTargetIndex;
        uint32_t depthIndex;
        uint32_t normalRIndex;

        uint32_t screenWidth;
        uint32_t screenHeight;
        float time;
        uint32_t _padding1;

        GunShot gunShots[4];
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
    const uint32_t MAX_GUN_SHOTS = 4;
    int next_gunshot_index = 0; // Points to where the next shot will be added

    void CreatePipeline();
    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();
};