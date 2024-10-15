#include "frame_graph.hpp"
#include "vulkan_brain.hpp"
#include "gpu_resources.hpp"
#include "spdlog/spdlog.h"

FrameGraphNodeCreation& FrameGraphNodeCreation::SetRenderPass(const FrameGraphRenderPass* renderPass)
{
    this->renderPass = renderPass;
    return *this;
}

FrameGraphNodeCreation& FrameGraphNodeCreation::AddInput(ResourceHandle<Image> image, FrameGraphResourceType type)
{
    FrameGraphResourceCreation& creation = inputs.emplace_back();
    creation.type = type;
    creation.info.image.handle = image;
    return *this;
}

FrameGraphNodeCreation& FrameGraphNodeCreation::AddInput(ResourceHandle<Buffer> buffer, FrameGraphResourceType type)
{
    FrameGraphResourceCreation& creation = inputs.emplace_back();
    creation.type = type;
    creation.info.buffer.handle = buffer;
    return *this;
}

FrameGraphNodeCreation& FrameGraphNodeCreation::AddOutput(ResourceHandle<Image> image, FrameGraphResourceType type)
{
    FrameGraphResourceCreation& creation = outputs.emplace_back();
    creation.type = type;
    creation.info.image.handle = image;
    return *this;
}

FrameGraphNodeCreation& FrameGraphNodeCreation::AddOutput(ResourceHandle<Buffer> buffer, FrameGraphResourceType type)
{
    FrameGraphResourceCreation& creation = outputs.emplace_back();
    creation.type = type;
    creation.info.buffer.handle = buffer;
    return *this;
}

FrameGraphNodeCreation& FrameGraphNodeCreation::SetIsEnabled(bool isEnabled)
{
    this->isEnabled = isEnabled;
    return *this;
}

FrameGraphNodeCreation& FrameGraphNodeCreation::SetName(std::string_view name)
{
    this->name = name;
    return *this;
}

FrameGraph::FrameGraph(const VulkanBrain& brain) :
    _brain(brain)
{
}

void FrameGraph::Build()
{
    // Clear the previous edges
    for (auto& node : _nodes)
    {
        node.edges.clear();
    }

    // First compute edges between nodes
    for (uint32_t i = 0; i < _nodes.size(); i++)
    {
        if (!_nodes[i].isEnabled)
        {
            continue;
        }

        ComputeNodeEdges(_nodes[i], i);
    }

    // Now run the topological sort
    enum class NodeStatus
    {
        eNotProcessed,
        eVisited,
        eAdded
    };

    std::vector<FrameGraphNodeHandle> reverseSortedNodes{};
    reverseSortedNodes.reserve(_nodes.size());

    std::vector<NodeStatus> nodesStatus(_nodes.size(), NodeStatus::eNotProcessed);

    std::vector<FrameGraphNodeHandle> nodesToProcess {};
    nodesToProcess.reserve(_nodes.size());

    for (uint32_t i = 0; i < _nodes.size(); ++i)
    {
        if (!_nodes[i].isEnabled)
        {
            continue;
        }

        nodesToProcess.push_back(i);

        while (!nodesToProcess.empty())
        {
            const FrameGraphNodeHandle nodeHandle = nodesToProcess.back();

            // Outputs might link to multiple nodes, and we don’t want to add the producing node multiple times to the sorted node list.
            if (nodesStatus[nodeHandle] == NodeStatus::eAdded)
            {
                nodesToProcess.pop_back();
                continue;
            }

            // If the node we are currently processing has already been visited, and we got to it in the stack,
            // it means we processed all of its children, and it can be added to the list of sorted nodes.
            if (nodesStatus[nodeHandle] == NodeStatus::eVisited)
            {
                nodesStatus[nodeHandle] = NodeStatus::eAdded;

                reverseSortedNodes.push_back(nodeHandle);
                nodesToProcess.pop_back();
                continue;
            }

            nodesStatus[nodeHandle] = NodeStatus::eVisited;
            const FrameGraphNode& node = _nodes[nodeHandle];

            // If the node has no edges, it is a leaf node
            if (node.edges.empty())
            {
                continue;
            }

            // If the node is not a leaf node, add its children for processing
            for (const FrameGraphNodeHandle childNodeHandle : node.edges)
            {
                if (nodesStatus[childNodeHandle] == NodeStatus::eNotProcessed)
                {
                    nodesToProcess.push_back(childNodeHandle);
                }
            }
        }
    }

    assert(_nodes.size() == reverseSortedNodes.size() && "There is something really wrong if this happens, contact Marcin (:");

    _sortedNodes.clear();

    for (int32_t i = reverseSortedNodes.size() - 1; i >= 0; --i)
    {
        _sortedNodes.push_back(reverseSortedNodes[i]);
    }

    _nodes.clear();
}

void FrameGraph::Render(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
}

FrameGraph& FrameGraph::AddNode(const FrameGraphNodeCreation& creation)
{
    const FrameGraphNodeHandle nodeHandle = _nodes.size();
    FrameGraphNode& node = _nodes.emplace_back();
    node.name = creation.name;
    node.isEnabled = creation.isEnabled;

    const FrameGraphRenderPassHandle renderPassHandle = _renderPasses.size();
    _renderPasses.push_back(creation.renderPass);
    node.renderPass = renderPassHandle;

    for (const auto& resourceCreation : creation.outputs)
    {
        const FrameGraphResourceHandle resourceHandle = CreateOutputResource(resourceCreation, nodeHandle);
        node.outputs.push_back(resourceHandle);
    }

    for (const auto& resourceCreation : creation.inputs)
    {
        const FrameGraphResourceHandle resourceHandle = CreateInputResource(resourceCreation);
        node.inputs.push_back(resourceHandle);
    }

    return *this;
}

void FrameGraph::ComputeNodeEdges(const FrameGraphNode& node, FrameGraphNodeHandle nodeHandle)
{
    for (uint32_t i = 0; i < node.inputs.size(); ++i)
    {
        FrameGraphResource& inputResource = _resources[node.inputs[i]];

        assert(_outputResourcesMap.find(inputResource.name) != _outputResourcesMap.end() && "Requested resource is not produced by any node.");

        const FrameGraphResourceHandle outputResourceHandle = _outputResourcesMap[inputResource.name];
        const FrameGraphResource& outputResource = _resources[outputResourceHandle];

        inputResource.producer = outputResource.producer;
        inputResource.info = outputResource.info;
        inputResource.output = outputResource.output;

        FrameGraphNode& parentNode = _nodes[inputResource.producer];
        parentNode.edges.push_back(nodeHandle);

        // spdlog::info("Adding edge from {} [{}] to {} [{}]\n", parentNode.name.c_str(), inputResource.producer, node.name.c_str(), nodeHandle);
    }
}

FrameGraphResourceHandle FrameGraph::CreateOutputResource(const FrameGraphResourceCreation& creation, FrameGraphNodeHandle producer)
{
    assert(HasFlags(creation.type, FrameGraphResourceType::eNone) && "FrameGraphResource must have a type.");

    const FrameGraphResourceHandle resourceHandle = _resources.size();
    FrameGraphResource& resource = _resources.emplace_back();
    resource.type = creation.type;
    resource.name = GetResourceName(creation);

    if (!HasFlags(creation.type, FrameGraphResourceType::eReference))
    {
        assert(_outputResourcesMap.find(GetResourceName(creation)) == _outputResourcesMap.end() && "Multiple nodes produce the same resource. Please use the eReference resource type to reference resources produced by multiple nodes.");

        resource.info = creation.info;
        resource.output = resourceHandle;
        resource.producer = producer;

        _outputResourcesMap.emplace(resource.name, resourceHandle);
    }

    return resourceHandle;
}

FrameGraphResourceHandle FrameGraph::CreateInputResource(const FrameGraphResourceCreation& creation)
{
    assert(HasFlags(creation.type, FrameGraphResourceType::eNone) && "FrameGraphResource must have a type.");

    const FrameGraphResourceHandle resourceHandle = _resources.size();
    FrameGraphResource& resource = _resources.emplace_back();
    resource.type = creation.type;
    resource.name = GetResourceName(creation);

    return resourceHandle;
}

// TODO: Use templates to avoid if statements?
std::string FrameGraph::GetResourceName(const FrameGraphResourceCreation& creation)
{
    if (HasFlags(creation.type, FrameGraphResourceType::eAttachment | FrameGraphResourceType::eTexture))
    {
        const Image* image = _brain.GetImageResourceManager().Access(creation.info.image.handle);
        return image->name;
    }

    if (HasFlags(creation.type, FrameGraphResourceType::eBuffer))
    {
        const Buffer* buffer = _brain.GetBufferResourceManager().Access(creation.info.buffer.handle);
        return buffer->name;
    }

    assert(false && "Unsupported resource type!");
    return "";
}