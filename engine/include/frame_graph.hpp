#pragma once

struct RenderSceneDescription;

enum class FrameGraphResourceType
{
    // Frame buffer that is being rendered to
    eAttachment,

    // Image that is read during the render pass
    eTexture,

    // Buffer of data that we can write to or read from
    eBuffer,

    // Type exclusively used to ensure correct node ordering // TODO: Example for others?
    eReference,

    eNone
};

using FrameGraphNodeHandle = uint32_t;
using FrameGraphResourceHandle = uint32_t;
using FrameGraphRenderPassHandle = uint32_t;

struct FrameGraphResourceInfo
{
    union
    {
        struct
        {
            ResourceHandle<Buffer> buffer = ResourceHandle<Buffer>::Invalid();
        };

        struct
        {
            ResourceHandle<Image> image = ResourceHandle<Image>::Invalid();
        };
    };
};

struct FrameGraphResourceCreation
{
    FrameGraphResourceType type = FrameGraphResourceType::eNone;
    FrameGraphResourceInfo info {};

    std::string name {};
};

struct FrameGraphResource
{
    FrameGraphResourceType type = FrameGraphResourceType::eNone;
    FrameGraphResourceInfo info {};

    FrameGraphNodeHandle producer = 0;
    FrameGraphResourceHandle output = 0;

    std::string name {};
};

struct FrameGraphRenderPass
{
    virtual ~FrameGraphRenderPass() = default;
    virtual void Render(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) = 0;
};

struct FrameGraphNodeCreation
{
    const FrameGraphRenderPass* renderPass = nullptr;

    std::vector<FrameGraphResourceCreation> inputs {};
    std::vector<FrameGraphResourceCreation> outputs {};

    bool isEnabled = true;
    std::string name {};

    FrameGraphNodeCreation& SetRenderPass(const FrameGraphRenderPass* renderPass);

    // TODO: Add input and output helpers

    FrameGraphNodeCreation& SetIsEnabled(bool isEnabled);
    FrameGraphNodeCreation& SetName(const std::string& name);
};

struct FrameGraphNode
{
    FrameGraphRenderPassHandle renderPass = 0;

    std::vector<FrameGraphResourceHandle> inputs {};
    std::vector<FrameGraphResourceHandle> outputs {};

    std::vector<FrameGraphNodeHandle> edges {};

    bool isEnabled = true;
    std::string name {};
};

class FrameGraph
{
public:
    FrameGraph();

    // Builds the graph from the node inputs.
    void Build();

    // Calls all the render passes from the built graph.
    void Render(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene);

    // Adds a new node to the graph. To actually use the node, you also need to build the graph, by calling FrameGraph::Build().
    FrameGraph& AddNode(const FrameGraphNodeCreation& creation);

private:
    std::unordered_map<std::string, FrameGraphResourceHandle> _outputResourcesMap {};

    std::vector<FrameGraphResource> _resources {};
    std::vector<FrameGraphNode> _nodes {};
    std::vector<const FrameGraphRenderPass*> _renderPasses {};

    std::vector<FrameGraphNodeHandle> _sortedNodes {};

    void ComputeNodeEdges(const FrameGraphNode& node, FrameGraphNodeHandle nodeHandle);
    FrameGraphResourceHandle CreateOutputResource(const FrameGraphResourceCreation& creation, FrameGraphNodeHandle producer);
    FrameGraphResourceHandle CreateInputResource(const FrameGraphResourceCreation& creation);
};