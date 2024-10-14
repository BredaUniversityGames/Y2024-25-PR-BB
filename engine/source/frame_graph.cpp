#include "frame_graph.hpp"

FrameGraph::FrameGraph()
{

}

void FrameGraph::Build()
{
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

    std::vector<FrameGraphNodeHandle> reverseSortedNodes(_nodes.size());
    std::vector<NodeStatus> nodesStatus(_nodes.size(), NodeStatus::eNotProcessed);
    std::vector<FrameGraphNodeHandle> nodesToProcess{};
    nodesToProcess.reserve(_nodes.size());

    for (int i = 0; i < _nodes.size(); i++)
    {
        if (!_nodes[i].isEnabled)
        {
            continue;
        }

        nodesToProcess.push_back(i);

        while(!nodesToProcess.empty())
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
                nodesStatus[nodeHandle] == NodeStatus::eAdded;

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
                if(nodesStatus[childNodeHandle] != NodeStatus::eNotProcessed)
                {
                    nodesToProcess.push_back(childNodeHandle);
                }
            }
        }
    }

    _sortedNodes.clear();

    for (uint32_t i = reverseSortedNodes.size() - 1; i >= 0; --i)
    {
        _sortedNodes.push_back(reverseSortedNodes[i]);
    }
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
    }
}

FrameGraphResourceHandle FrameGraph::CreateOutputResource(const FrameGraphResourceCreation& creation, FrameGraphNodeHandle producer)
{
    assert(creation.type == FrameGraphResourceType::eNone && "FrameGraphResource must have a type.");

    const FrameGraphResourceHandle resourceHandle = _resources.size();
    FrameGraphResource& resource = _resources.emplace_back();
    resource.type = creation.type;
    resource.name = creation.name;

    if (creation.type != FrameGraphResourceType::eReference)
    {
        resource.info = creation.info;
        resource.output = resourceHandle;
        resource.producer = producer;

        _outputResourcesMap.emplace(creation.name, resourceHandle);
    }

    return resourceHandle;
}

FrameGraphResourceHandle FrameGraph::CreateInputResource(const FrameGraphResourceCreation& creation)
{
    assert(creation.type == FrameGraphResourceType::eNone && "FrameGraphResource must have a type.");

    const FrameGraphResourceHandle resourceHandle = _resources.size();
    FrameGraphResource& resource = _resources.emplace_back();
    resource.type = creation.type;
    resource.name = creation.name;

    return resourceHandle;
}