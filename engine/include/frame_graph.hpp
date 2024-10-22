#pragma once

#include "vulkan_brain.hpp"
#include "resource_manager.hpp"

struct RenderSceneDescription;
struct Image;
struct Buffer;

enum class FrameGraphResourceType : uint8_t
{
    eNone = 0 << 0,

    // Frame buffer that is being rendered to
    eAttachment = 1 << 0,

    // Image that is read during the render pass
    eTexture = 1 << 0,

    // Buffer of data that we can write to or read from
    eBuffer = 1 << 1,

    // Type exclusively used to ensure correct node ordering when the pass does not actually use the resource // TODO: Example for others?
    eReference = 1 << 2,
};

inline FrameGraphResourceType operator|(FrameGraphResourceType lhs, FrameGraphResourceType rhs)
{
    return static_cast<FrameGraphResourceType>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline FrameGraphResourceType operator&(FrameGraphResourceType lhs, FrameGraphResourceType rhs)
{
    return static_cast<FrameGraphResourceType>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

inline bool HasFlags(FrameGraphResourceType lhs, FrameGraphResourceType rhs)
{
    return (lhs & rhs) == rhs;
}

using FrameGraphNodeHandle = uint32_t;
using FrameGraphResourceHandle = uint32_t;

struct FrameGraphResourceInfo
{
    union
    {
        struct
        {
            ResourceHandle<Buffer> handle = ResourceHandle<Buffer>::Invalid();
            vk::PipelineStageFlags stageUsage;
        } buffer;

        struct
        {
            ResourceHandle<Image> handle = ResourceHandle<Image>::Invalid();
        } image;
    };
};

struct FrameGraphResourceCreation
{
    FrameGraphResourceType type = FrameGraphResourceType::eNone;
    FrameGraphResourceInfo info {};
};

struct FrameGraphResource
{
    FrameGraphResourceType type = FrameGraphResourceType::eNone;
    FrameGraphResourceInfo info {};

    FrameGraphNodeHandle producer = 0;
    FrameGraphResourceHandle output = 0;

    std::string name {};
};

class FrameGraphRenderPass
{
public:
    virtual ~FrameGraphRenderPass() = default;
    virtual void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) const = 0;
};

struct ImageMemoryBarrier
{
    ImageMemoryBarrier(const Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::ImageAspectFlagBits imageAspect = vk::ImageAspectFlagBits::eColor);

    vk::PipelineStageFlags srcStage{};
    vk::PipelineStageFlags dstStage{};
    vk::ImageMemoryBarrier barrier{};
};

struct BufferMemoryBarrier
{
    vk::PipelineStageFlags srcStage{};
    vk::PipelineStageFlags dstStage{};
    vk::BufferMemoryBarrier barrier{};
};

struct FrameGraphNodeCreation
{
    const FrameGraphRenderPass* renderPass = nullptr;

    std::vector<FrameGraphResourceCreation> inputs {};
    std::vector<FrameGraphResourceCreation> outputs {};

    bool isEnabled = true;
    std::string name {};
    glm::vec3 debugLabelColor = glm::vec3(0.0f);

    FrameGraphNodeCreation& SetRenderPass(const FrameGraphRenderPass* renderPass);

    FrameGraphNodeCreation& AddInput(ResourceHandle<Image> image, FrameGraphResourceType type);
    FrameGraphNodeCreation& AddInput(ResourceHandle<Buffer> buffer, FrameGraphResourceType type, vk::PipelineStageFlags stageUsage);

    FrameGraphNodeCreation& AddOutput(ResourceHandle<Image> image, FrameGraphResourceType type);
    FrameGraphNodeCreation& AddOutput(ResourceHandle<Buffer> buffer, FrameGraphResourceType type, vk::PipelineStageFlags stageUsage);

    FrameGraphNodeCreation& SetIsEnabled(bool isEnabled);
    FrameGraphNodeCreation& SetName(std::string_view name);
    FrameGraphNodeCreation& SetDebugLabelColor(const glm::vec3& color);
};

struct FrameGraphNode
{
    const FrameGraphRenderPass* renderPass = nullptr;

    std::vector<FrameGraphResourceHandle> inputs {};
    std::vector<FrameGraphResourceHandle> outputs {};

    std::vector<FrameGraphNodeHandle> edges {};

    std::vector<ImageMemoryBarrier> imageMemoryBarriers {};
    std::vector<BufferMemoryBarrier> bufferMemoryBarriers {};

    vk::Viewport viewport{};
    vk::Rect2D scissor{};

    bool isEnabled = true;
    std::string name {};
    glm::vec3 debugLabelColor = glm::vec3(0.0f);
};

class FrameGraph
{
public:
    FrameGraph(const VulkanBrain& brain);

    // Builds the graph from the node inputs.
    void Build();

    // Calls all the render passes from the built graph.
    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene);

    // Adds a new node to the graph. To actually use the node, you also need to build the graph, by calling FrameGraph::Build().
    FrameGraph& AddNode(const FrameGraphNodeCreation& creation);

private:
    const VulkanBrain& _brain;

    std::unordered_map<std::string, FrameGraphResourceHandle> _outputResourcesMap {};

    std::vector<FrameGraphResource> _resources {};
    std::vector<FrameGraphNode> _nodes {};

    std::vector<FrameGraphNodeHandle> _sortedNodes {};

    void ProcessNodes();
    void ComputeNodeEdges(const FrameGraphNode& node, FrameGraphNodeHandle nodeHandle);
    void ComputeNodeMemoryBarriers(FrameGraphNode& node);
    void SortGraph();
    FrameGraphResourceHandle CreateOutputResource(const FrameGraphResourceCreation& creation, FrameGraphNodeHandle producer);
    FrameGraphResourceHandle CreateInputResource(const FrameGraphResourceCreation& creation);
    std::string GetResourceName(const FrameGraphResourceCreation& creation);
};