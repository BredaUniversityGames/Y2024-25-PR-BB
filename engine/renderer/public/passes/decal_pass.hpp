#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"

#include "vulkan/vulkan.hpp"
#include <memory>

class GraphicsContext;

struct Buffer;

constexpr uint32_t MAX_DECALS = 32;

struct alignas(16) DecalData
{
    glm::mat4 invModel;
    glm::vec3 orientation;
    uint32_t albedoIndex;
};

class DecalPass final : public FrameGraphRenderPass
{
public:
    DecalPass(const std::shared_ptr<GraphicsContext>& context, const Settings::Lighting& lightingSettings, const GBuffers& gBuffers);
    ~DecalPass() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    void SpawnDecal(glm::vec3 normal, glm::vec3 position, glm::vec2 size, std::string albedoName);
    void ResetDecals();

    NON_COPYABLE(DecalPass);
    NON_MOVABLE(DecalPass);

private:
    struct PushConstants
    {
        uint32_t albedoRMIndex;
        uint32_t depthIndex;
        glm::uvec2 screenSize;
        float decalNormalThreshold;
    } _pushConstants;

    std::shared_ptr<GraphicsContext> _context;
    const Settings::Lighting& _lightingSettings;
    const GBuffers& _gBuffers;

    glm::uvec2 _screenSize;

    ResourceHandle<GPUImage>& GetDecalImage(std::string fileName);
    std::unordered_map<std::string, ResourceHandle<GPUImage>> _decalImages;

    std::array<DecalData, 32> _decals;
    uint32_t _decalCount = 0;

    vk::Pipeline _pipeline;
    vk::PipelineLayout _pipelineLayout;

    std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT> _decalBuffers;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _decalDescriptorSets;
    vk::DescriptorSetLayout _decalDescriptorSetLayout;

    void UpdateDecals();
    void UpdateDecalBuffer(uint32_t currentFrame);

    void CreatePipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();
    void CreateBuffers();

    void UpdateDescriptorSet(uint32_t frameIndex);
};