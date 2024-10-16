#include "frame_graph.hpp"
#include "vulkan_brain.hpp"
#include "gpu_resources.hpp"
#include "vulkan_helper.hpp"
#include "glm/gtc/random.hpp"
#include "spdlog/spdlog.h"
#include "glm/gtx/range.hpp"

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
    // First compute edges between nodes
    ComputeEdges();

    // Now run the topological sort
    enum class NodeStatus : uint8_t
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

    assert(_nodes.size() >= reverseSortedNodes.size() && "There is something really wrong if this happens, this should never happen");

    _sortedNodes.clear();

    for (int32_t i = reverseSortedNodes.size() - 1; i >= 0; --i)
    {
        _sortedNodes.push_back(reverseSortedNodes[i]);
    }

    _nodes.clear();
}

void FrameGraph::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    for (const FrameGraphNodeHandle nodeHandle : _sortedNodes)
    {
        const FrameGraphNode& node = _nodes[nodeHandle];

        util::BeginLabel(commandBuffer, node.name, glm::linearRand(glm::vec3(0.0f), glm::vec3(1.0f)), _brain.dldi);

        // Handle input memory barriers
        for (const FrameGraphResourceHandle inputHandle : node.inputs)
        {
            const FrameGraphResource& resource = _resources[inputHandle];

            if (resource.type == FrameGraphResourceType::eTexture)
            {
                const Image* texture = _brain.GetImageResourceManager().Access(resource.info.image.handle);

                if (texture->flags & vk::ImageUsageFlagBits::eDepthStencilAttachment)
                {
                    util::TransitionImageLayout(commandBuffer, texture->image, texture->format, vk::ImageLayout::eDepthStencilAttachmentOptimal,
                        vk::ImageLayout::eShaderReadOnlyOptimal, texture->layers, 0, texture->mips, vk::ImageAspectFlagBits::eDepth);
                }
                else
                {
                    util::TransitionImageLayout(commandBuffer, texture->image, texture->format,
                        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
                }
            }
            else if (resource.type == FrameGraphResourceType::eBuffer)
            {
                // TODO: Handle barriers for buffers
            }
        }

        // Handle output memory barriers
        for (const FrameGraphResourceHandle outputHandle : node.outputs)
        {
            const FrameGraphResource& resource = _resources[outputHandle];

            if (resource.type == FrameGraphResourceType::eAttachment)
            {
                const Image* attachment = _brain.GetImageResourceManager().Access(resource.info.image.handle);

                if (attachment->flags & vk::ImageUsageFlagBits::eDepthStencilAttachment)
                {
                    util::TransitionImageLayout(commandBuffer, attachment->image, attachment->format, vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eDepthStencilAttachmentOptimal, attachment->layers, 0, attachment->mips, vk::ImageAspectFlagBits::eDepth);
                }
                else
                {
                    util::TransitionImageLayout(commandBuffer, attachment->image, attachment->format,
                        vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
                }
            }
        }

        // TODO: Viewport and scissor?

        node.renderPass->RecordCommands(commandBuffer, currentFrame, scene);

        util::EndLabel(commandBuffer, _brain.dldi);
    }
}

FrameGraph& FrameGraph::AddNode(const FrameGraphNodeCreation& creation)
{
    const FrameGraphNodeHandle nodeHandle = _nodes.size();
    FrameGraphNode& node = _nodes.emplace_back();
    node.name = creation.name;
    node.isEnabled = creation.isEnabled;
    node.renderPass = creation.renderPass;

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

void FrameGraph::ComputeEdges()
{
    for (auto& node : _nodes)
    {
        node.edges.clear();
    }

    for (uint32_t i = 0; i < _nodes.size(); i++)
    {
        if (!_nodes[i].isEnabled)
        {
            continue;
        }

        ComputeNodeEdges(_nodes[i], i);
    }
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

        spdlog::info("Adding edge from {} [{}] to {} [{}]\n", parentNode.name.c_str(), inputResource.producer, node.name.c_str(), nodeHandle);
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