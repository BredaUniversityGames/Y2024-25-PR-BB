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
    VolumetricPass(const std::shared_ptr<GraphicsContext>& context, const Settings::Tonemapping& settings, ResourceHandle<GPUImage> hdrTarget, ResourceHandle<GPUImage> bloomTarget, ResourceHandle<GPUImage> outputTarget, const SwapChain& _swapChain, const GBuffers& gBuffers, const BloomSettings& bloomSettings, ECSModule& ecs);
    ~VolumetricPass() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    void AddGunShot(const glm::vec3 origin, const glm::vec3 direction)
    {
        // Place the new GunShot at the current index
        gunShots[next_gunshot_index].origin = glm::vec4(origin, 0.3);
        gunShots[next_gunshot_index].direction = glm::vec4(direction, 0.3);

        next_gunshot_index = (next_gunshot_index + 1) % gunShots.size();
    }

    void AddPlayerPos(const glm::vec3 position)
    {
        // Place the new player position at the current index
        int index = next_player_pos_index;
        if (index == 0)
        {
            index = playerTrailPositions.size() - 1;
        }
        else
        {
            index = next_player_pos_index - 1;
        }
        glm::vec4 currentPos = playerTrailPositions[index];
        const float distanceCheck = glm::distance(position, glm::vec3(currentPos));
        if (distanceCheck < 1.09f)
        {
            return; // too close to the last position
        }
        playerTrailPositions[next_player_pos_index] = glm::vec4(position, 1.1);

        next_player_pos_index = (next_player_pos_index + 1) % playerTrailPositions.size();
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

    std::array<GunShot, 8> gunShots;
    std::array<glm::vec4, 24> playerTrailPositions;
    ResourceHandle<Buffer> _fogTrailsBuffer;
    vk::DescriptorSetLayout _fogTrailsDescriptorSetLayout;
    vk::DescriptorSet _fogTrailsDescriptorSet;

    const BloomSettings& _bloomSettings;
    float timePassed = 0.0f;
    const uint32_t MAX_GUN_SHOTS = 8;
    int next_gunshot_index = 0; // Points to where the next shot will be added
    int next_player_pos_index = 0; // Points to where the next player position will be added

    float _playerPosCounterMs = 0;
    const float _playerPosCounterMaxMs = 300;

    ECSModule& _ecs;
    void CreatePipeline();
    void UpdateFogTrailsBuffer();
    void CreateFogTrailsBuffer();
    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();
};