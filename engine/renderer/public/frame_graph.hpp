#pragma once
#include "common.hpp"
#include "enum_utils.hpp"
#include "resource_manager.hpp"
#include "swap_chain.hpp"
#include <glm/vec3.hpp>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vulkan/vulkan.hpp>

struct RenderSceneDescription;
struct Buffer;
struct GPUImage;
class GraphicsContext;

enum class FrameGraphRenderPassType : uint8_t
{
    eGraphics,
    eCompute,
};

enum class FrameGraphResourceType : uint8_t
{
    eNone = 1 << 0,

    // Frame buffer that is being rendered to
    eAttachment = 1 << 1,

    // Image that is read during the render pass
    eTexture = 1 << 2,

    // Buffer of data that we can write to or read from
    eBuffer = 1 << 3,

    // Type exclusively used to ensure correct node ordering when the pass does not actually use the resource
    eReference = 1 << 4,
};
GENERATE_ENUM_FLAG_OPERATORS(FrameGraphResourceType)

using FrameGraphNodeHandle = uint32_t;
using FrameGraphResourceHandle = uint32_t;

struct FrameGraphResourceInfo
{
    struct StageBuffer
    {
        ResourceHandle<Buffer> handle = ResourceHandle<Buffer>::Null();
        vk::PipelineStageFlags2 stageUsage;
    };

    FrameGraphResourceInfo(const std::variant<std::monostate, StageBuffer, ResourceHandle<GPUImage>>& resource)
        : resource(resource)
    {
    }

    using Resource = std::variant<std::monostate, StageBuffer, ResourceHandle<GPUImage>>;
    Resource resource {};
    bool allowSimultaneousWrites = false;
};

struct FrameGraphResourceCreation
{
    FrameGraphResourceCreation(const std::variant<std::monostate, FrameGraphResourceInfo::StageBuffer, ResourceHandle<GPUImage>>& resource)
        : info(resource)
    {
    }

    FrameGraphResourceType type = FrameGraphResourceType::eNone;
    FrameGraphResourceInfo info;
};

struct FrameGraphResource
{
    FrameGraphResource(const std::variant<std::monostate, FrameGraphResourceInfo::StageBuffer, ResourceHandle<GPUImage>>& resource)
        : info(resource)
    {
    }

    FrameGraphResourceType type = FrameGraphResourceType::eNone;
    FrameGraphResourceInfo info;
    uint32_t version = 0;

    FrameGraphNodeHandle producer = 0;
    FrameGraphResourceHandle output = 0;

    // Includes the name of the resource + "_v-" + version
    std::string versionedName {};
};

class FrameGraphRenderPass
{
public:
    virtual ~FrameGraphRenderPass() = default;
    virtual void RecordCommands(vk::CommandBuffer commandBuffer, MAYBE_UNUSED uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene) = 0;
};

struct FrameGraphNodeCreation
{
    FrameGraphRenderPassType queueType = FrameGraphRenderPassType::eGraphics;
    FrameGraphRenderPass& renderPass;

    std::vector<FrameGraphResourceCreation> inputs {};
    std::vector<FrameGraphResourceCreation> outputs {};

    bool isEnabled = true;
    std::string name {};
    glm::vec3 debugLabelColor = glm::vec3(0.0f);

    FrameGraphNodeCreation(FrameGraphRenderPass& renderPass, FrameGraphRenderPassType queueType = FrameGraphRenderPassType::eGraphics);

    FrameGraphNodeCreation& AddInput(ResourceHandle<GPUImage> image, FrameGraphResourceType type);
    FrameGraphNodeCreation& AddInput(ResourceHandle<Buffer> buffer, FrameGraphResourceType type, vk::PipelineStageFlags2 stageUsage);

    FrameGraphNodeCreation& AddOutput(ResourceHandle<GPUImage> image, FrameGraphResourceType type, bool allowSimultaneousWrites = false);
    FrameGraphNodeCreation& AddOutput(ResourceHandle<Buffer> buffer, FrameGraphResourceType type, vk::PipelineStageFlags2 stageUsage, bool allowSimultaneousWrites = false);

    FrameGraphNodeCreation& SetIsEnabled(bool isEnabled);
    FrameGraphNodeCreation& SetName(std::string_view name);
    FrameGraphNodeCreation& SetDebugLabelColor(const glm::vec3& color);
};

struct FrameGraphNode
{
    FrameGraphRenderPassType queueType = FrameGraphRenderPassType::eGraphics;
    FrameGraphRenderPass& renderPass;

    std::vector<FrameGraphResourceHandle> inputs {};
    std::vector<FrameGraphResourceHandle> outputs {};

    std::vector<FrameGraphNodeHandle> edges {};

    vk::DependencyInfo dependencyInfo {};
    std::vector<vk::ImageMemoryBarrier2> imageMemoryBarriers {};
    std::vector<vk::BufferMemoryBarrier2> bufferMemoryBarriers {};

    vk::Viewport viewport {};
    vk::Rect2D scissor {};

    bool isEnabled = true;
    std::string name {};
    glm::vec3 debugLabelColor = glm::vec3(0.0f);

    FrameGraphNode(FrameGraphRenderPass& renderPass, FrameGraphRenderPassType queueType);
};

class FrameGraph
{
public:
    FrameGraph(const std::shared_ptr<GraphicsContext>& context, const SwapChain& swapChain);

    // Builds the graph from the node inputs.
    void Build();

    // Calls all the render passes from the built graph.
    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene);

    // Adds a new node to the graph. To actually use the node, you also need to build the graph, by calling FrameGraph::Build().
    FrameGraph& AddNode(const FrameGraphNodeCreation& creation);

private:
    // Used to track resource states when generating memory barriers.
    enum class ResourceState : uint8_t
    {
        eFirstOuput,
        eReusedOutputAfterOutput,
        eReusedOutputAfterInput,
        eInput,
    };

    std::shared_ptr<GraphicsContext> _context;
    const SwapChain& _swapChain;

    // Used for fast lookup when linking input to output resources
    std::unordered_map<std::string, FrameGraphResourceHandle> _outputResourcesMap {};
    // Used for fast lookup when trying to find the newest version of a reused output resource
    std::unordered_map<std::string, FrameGraphResourceHandle> _newestVersionedResourcesMap {};

    std::vector<FrameGraphResource> _resources {};
    std::vector<FrameGraphNode> _nodes {};

    std::vector<FrameGraphNodeHandle> _sortedNodes {};

    void ProcessNodes();
    void ComputeNodeEdges(const FrameGraphNode& node, FrameGraphNodeHandle nodeHandle);
    void ComputeNodeViewportAndScissor(FrameGraphNodeHandle nodeHandle);
    void CreateMemoryBarriers();
    void CreateImageBarrier(const FrameGraphResource& resource, ResourceState state, vk::ImageMemoryBarrier2& barrier) const;
    void CreateBufferBarrier(const FrameGraphResource& resource, ResourceState state, vk::BufferMemoryBarrier2& barrier) const;
    void SortGraph();
    FrameGraphResourceHandle CreateOutputResource(const FrameGraphResourceCreation& creation, FrameGraphNodeHandle producer);
    FrameGraphResourceHandle CreateInputResource(const FrameGraphResourceCreation& creation);
    const std::string& GetResourceName(FrameGraphResourceType type, const FrameGraphResourceInfo::Resource& resource) const;
};