#pragma once

#include "frame_graph.hpp"
#include "gbuffers.hpp"
#include "resource_manager.hpp"

#include "vulkan/vulkan.hpp"
#include <memory>

class GraphicsContext;

struct Buffer;

static constexpr uint32_t MAX_DECALS = 64;

struct DecalData
{
    glm::vec3 position = glm::vec3(0.0f);
};

class DecalsPass final : public FrameGraphRenderPass
{
public:
    DecalsPass(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers);
    ~DecalsPass() final;

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

    NON_COPYABLE(DecalsPass);
    NON_MOVABLE(DecalsPass);

private:
    struct DecalsPushConstant
    {
        uint32_t decalCount;
    } _decalsPushConstant;

    std::shared_ptr<GraphicsContext> _context;
    const GBuffers& _gBuffers;

    std::vector<DecalData> _newDecals;
    uint32_t _decalCount = 0;
    uint32_t _decalOffset = 0;

    vk::Pipeline _pipeline;
    vk::PipelineLayout _pipelineLayout;

    ResourceHandle<Buffer> _decalsBuffer;
    vk::DescriptorSet _decalsBufferDescriptorSet;
    vk::DescriptorSetLayout _decalsBufferDescriptorSetLayout;

    void UpdateDecals();

    void CreatePipeline();
    void CreateDescriptorSetLayouts();
    void CreateDescriptorSets();
    void CreateBuffers();

    void UpdateBuffersDescriptorSets();
};