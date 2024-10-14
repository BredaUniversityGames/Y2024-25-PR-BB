#include "frame_graph.hpp"

FrameGraph::FrameGraph()
{

}

void FrameGraph::Build()
{
    for (uint32_t i = 0; i < _nodes.size(); i++)
    {
        if (_nodes[i].isEnabled)
        {
            continue;
        }

        ComputeNodeEdges(_nodes[i], i);
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
    resource.info = creation.info;

    return resourceHandle;
}