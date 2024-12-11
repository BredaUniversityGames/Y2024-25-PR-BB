#include "frame_graph.hpp"

#include <glm/gtc/random.hpp>
#include <glm/gtx/range.hpp>

#include "gpu_resources.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

FrameGraphNodeCreation::FrameGraphNodeCreation(FrameGraphRenderPass& renderPass, FrameGraphRenderPassType queueType)
    : queueType(queueType)
    , renderPass(renderPass)
{
}

FrameGraphNodeCreation& FrameGraphNodeCreation::AddInput(ResourceHandle<GPUImage> image, FrameGraphResourceType type)
{
    FrameGraphResourceCreation& creation = inputs.emplace_back(image);
    creation.type = type;

    return *this;
}

FrameGraphNodeCreation& FrameGraphNodeCreation::AddInput(ResourceHandle<Buffer> buffer, FrameGraphResourceType type, vk::PipelineStageFlags2 stageUsage)
{
    FrameGraphResourceCreation& creation = inputs.emplace_back(FrameGraphResourceInfo::StageBuffer { .handle = buffer, .stageUsage = stageUsage });
    creation.type = type;

    return *this;
}

FrameGraphNodeCreation& FrameGraphNodeCreation::AddOutput(ResourceHandle<GPUImage> image, FrameGraphResourceType type, bool allowSimultaneousWrites)
{
    FrameGraphResourceCreation& creation = outputs.emplace_back(image);
    creation.type = type;
    creation.info.allowSimultaneousWrites = allowSimultaneousWrites;

    return *this;
}

FrameGraphNodeCreation& FrameGraphNodeCreation::AddOutput(ResourceHandle<Buffer> buffer, FrameGraphResourceType type, vk::PipelineStageFlags2 stageUsage,  bool allowSimultaneousWrites)
{
    FrameGraphResourceCreation& creation = outputs.emplace_back(FrameGraphResourceInfo::StageBuffer { .handle = buffer, .stageUsage = stageUsage });
    creation.type = type;
    creation.info.allowSimultaneousWrites = allowSimultaneousWrites;

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

FrameGraphNodeCreation& FrameGraphNodeCreation::SetDebugLabelColor(const glm::vec3& color)
{
    this->debugLabelColor = color;
    return *this;
}

FrameGraphNode::FrameGraphNode(FrameGraphRenderPass& renderPass, FrameGraphRenderPassType queueType)
    : queueType(queueType)
    , renderPass(renderPass)
{
}

FrameGraph::FrameGraph(const std::shared_ptr<GraphicsContext>& context, const SwapChain& swapChain)
    : _context(context)
    , _swapChain(swapChain)
{
}

void FrameGraph::Build()
{
    // First compute edges between nodes and their viewports and scissors
    ProcessNodes();

    // Sort the graph based on the node connections made
    SortGraph();

    // Traverse sorted graph to create memory barriers, we do this after sorting to get rid of unneeded barriers
    CreateMemoryBarriers();
}

void FrameGraph::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    auto vkContext { _context->VulkanContext() };

    for (const FrameGraphNodeHandle nodeHandle : _sortedNodes)
    {
        const FrameGraphNode& node = _nodes[nodeHandle];

        util::BeginLabel(commandBuffer, node.name, node.debugLabelColor, vkContext->Dldi());

        // Place memory barriers
        commandBuffer.pipelineBarrier2(node.dependencyInfo);

        if (node.queueType == FrameGraphRenderPassType::eGraphics)
        {
            commandBuffer.setViewport(0, node.viewport);
            commandBuffer.setScissor(0, node.scissor);
        }

        node.renderPass.RecordCommands(commandBuffer, currentFrame, scene);

        util::EndLabel(commandBuffer, vkContext->Dldi());
    }
}

FrameGraph& FrameGraph::AddNode(const FrameGraphNodeCreation& creation)
{
    const FrameGraphNodeHandle nodeHandle = _nodes.size();
    FrameGraphNode& node = _nodes.emplace_back(creation.renderPass, creation.queueType);
    node.name = creation.name;
    node.debugLabelColor = creation.debugLabelColor;
    node.isEnabled = creation.isEnabled;

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

void FrameGraph::ProcessNodes()
{
    for (auto& node : _nodes)
    {
        node.edges.clear();
    }

    for (uint32_t i = 0; i < _nodes.size(); i++)
    {
        const FrameGraphNode& node = _nodes[i];

        if (!node.isEnabled)
        {
            continue;
        }

        ComputeNodeEdges(node, i);
        ComputeNodeViewportAndScissor(i);
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

        // spdlog::info("Adding edge from {} [{}] to {} [{}]\n", parentNode.name.c_str(), inputResource.producer, node.name.c_str(), nodeHandle);
    }
}

void FrameGraph::ComputeNodeViewportAndScissor(FrameGraphNodeHandle nodeHandle)
{
    auto resources { _context->Resources() };

    FrameGraphNode& node = _nodes[nodeHandle];

    // Only graphics queue render passes need a viewport and scissor
    if (node.queueType != FrameGraphRenderPassType::eGraphics)
    {
        return;
    }

    glm::uvec2 viewportSize = _swapChain.GetImageSize();

    for (const FrameGraphResourceHandle inputHandle : node.inputs)
    {
        const FrameGraphResource& resource = _resources[inputHandle];

        if (HasAnyFlags(resource.type, FrameGraphResourceType::eAttachment))
        {
            const GPUImage* attachment = resources->ImageResourceManager().Access(std::get<ResourceHandle<GPUImage>>(resource.info.resource));

            viewportSize.x = attachment->width;
            viewportSize.y = attachment->height;
            break;
        }
    }

    for (const FrameGraphResourceHandle outputHandle : node.outputs)
    {
        const FrameGraphResource& resource = _resources[outputHandle];

        // No references allowed, because output references shouldn't contribute to the pass
        if (resource.type == FrameGraphResourceType::eAttachment)
        {
            const GPUImage* attachment = resources->ImageResourceManager().Access(std::get<ResourceHandle<GPUImage>>(resource.info.resource));

            viewportSize.x = attachment->width;
            viewportSize.y = attachment->height;
            break;
        }
    }

    const glm::vec2 fViewportSize = viewportSize;
    node.viewport = vk::Viewport { 0.0f, 0.0f, fViewportSize.x, fViewportSize.y, 0.0f, 1.0f };
    node.scissor = vk::Rect2D { vk::Offset2D { 0, 0 }, vk::Extent2D { viewportSize.x, viewportSize.y } };
}

void FrameGraph::CreateMemoryBarriers()
{
    auto resources { _context->Resources() };

    std::unordered_map<std::string, ResourceState> resourceStates {};

    for (const FrameGraphNodeHandle nodeHandle : _sortedNodes)
    {
        FrameGraphNode& node = _nodes[nodeHandle];

        node.imageMemoryBarriers.clear();
        node.bufferMemoryBarriers.clear();

        // Handle input memory barriers
        for (const FrameGraphResourceHandle inputHandle : node.inputs)
        {
            const FrameGraphResource& resource = _resources[inputHandle];

            // If resource was used as an input already, there is no need to make a barrier again
            std::string resourceName = GetResourceName(resource.type, resource.info.resource);
            if (resourceStates[resourceName] == ResourceState::eInput)
            {
                continue;
            }
            resourceStates[resourceName] = ResourceState::eInput;

            if (resource.type == FrameGraphResourceType::eTexture)
            {
                vk::ImageMemoryBarrier2& barrier = node.imageMemoryBarriers.emplace_back();
                CreateImageBarrier(resource, resourceStates[resourceName], barrier);
            }
            else if (resource.type == FrameGraphResourceType::eBuffer)
            {
                vk::BufferMemoryBarrier2& barrier = node.bufferMemoryBarriers.emplace_back();
                CreateBufferBarrier(resource, resourceStates[resourceName], barrier);
            }
        }

        // Handle output memory barriers
        for (const FrameGraphResourceHandle outputHandle : node.outputs)
        {
            const FrameGraphResource& resource = _resources[outputHandle];

            std::string resourceName = GetResourceName(resource.type, resource.info.resource);
            auto itr = resourceStates.find(resourceName);
            if (itr != resourceStates.end())
            {
                resourceStates[resourceName] = itr->second == ResourceState::eInput ? ResourceState::eReusedOutputAfterInput : ResourceState::eReusedOutputAfterOutput;
            }
            else
            {
                resourceStates[resourceName] = ResourceState::eFirstOuput;
            }

            // We allow the user to force not having a barrier when he wants to write to a resource at the same time during multiple passes
            if (resourceStates[resourceName] == ResourceState::eReusedOutputAfterOutput && resource.info.allowSimultaneousWrites)
            {
                continue;
            }

            if (resource.type == FrameGraphResourceType::eAttachment)
            {
                vk::ImageMemoryBarrier2& barrier = node.imageMemoryBarriers.emplace_back();
                CreateImageBarrier(resource, resourceStates[resourceName], barrier);
            }
        }

        // Compile all barriers into dependency info to run them in a batch
        node.dependencyInfo.setImageMemoryBarrierCount(node.imageMemoryBarriers.size())
            .setPImageMemoryBarriers(node.imageMemoryBarriers.data())
            .setBufferMemoryBarrierCount(node.bufferMemoryBarriers.size())
            .setPBufferMemoryBarriers(node.bufferMemoryBarriers.data());
    }
}

void FrameGraph::CreateImageBarrier(const FrameGraphResource& resource, ResourceState state, vk::ImageMemoryBarrier2& barrier) const
{
    auto resources { _context->Resources() };
    const GPUImage* image = resources->ImageResourceManager().Access(std::get<ResourceHandle<GPUImage>>(resource.info.resource));

    switch (state)
    {
    case ResourceState::eFirstOuput:
    {
        if (image->flags & vk::ImageUsageFlagBits::eDepthStencilAttachment)
        {
            util::InitializeImageMemoryBarrier(barrier, image->image, image->format,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal,
                image->layers, 0, image->mips, vk::ImageAspectFlagBits::eDepth);
        }
        else
        {
            util::InitializeImageMemoryBarrier(barrier, image->image, image->format,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
                image->layers, 0, image->mips, vk::ImageAspectFlagBits::eColor);
        }
        break;
    }
    case ResourceState::eReusedOutputAfterOutput:
    {
        // TODO: Was not handeled before, but most likely needed
        break;
    }
    case ResourceState::eInput:
    {
        if (image->flags & vk::ImageUsageFlagBits::eDepthStencilAttachment)
        {
            util::InitializeImageMemoryBarrier(barrier, image->image, image->format,
                vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                image->layers, 0, image->mips, vk::ImageAspectFlagBits::eDepth);
        }
        else
        {
            util::InitializeImageMemoryBarrier(barrier, image->image, image->format,
                vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                image->layers, 0, image->mips, vk::ImageAspectFlagBits::eColor);
        }
        break;
    }
    case ResourceState::eReusedOutputAfterInput:
    default:
        bblog::error("[Frame Graph] Unsupported image resource usage: {}", magic_enum::enum_name(state));
        break;
    }
}

void FrameGraph::CreateBufferBarrier(const FrameGraphResource& resource, ResourceState state, vk::BufferMemoryBarrier2& barrier) const
{
    auto resources { _context->Resources() };
    auto stageBuffer = std::get<FrameGraphResourceInfo::StageBuffer>(resource.info.resource);
    const Buffer* buffer = resources->BufferResourceManager().Access(stageBuffer.handle);

    switch (state)
    {
    case ResourceState::eFirstOuput:
        break;
    case ResourceState::eInput:
    {
        // Get the buffer created before here and create barrier based on its stage usage
        const FrameGraphResourceHandle outputResourceHandle = _outputResourcesMap.at(resource.name);
        const FrameGraphResource& outputResource = _resources[outputResourceHandle];

        barrier.srcStageMask = std::get<FrameGraphResourceInfo::StageBuffer>(outputResource.info.resource).stageUsage;
        barrier.dstStageMask = stageBuffer.stageUsage;

        barrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
        barrier.dstAccessMask = vk::AccessFlagBits2::eMemoryRead; // TODO: Distinguish between VK_ACCESS_INDIRECT_COMMAND_READ_BIT and VK_ACCESS_SHADER_READ_BIT
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.buffer = buffer->buffer;
        barrier.offset = 0;
        barrier.size = vk::WholeSize;
        break;
    }
    case ResourceState::eReusedOutputAfterOutput:
    case ResourceState::eReusedOutputAfterInput:
    default:
        bblog::error("[Frame Graph] Unsupported buffer resource usage: {}", magic_enum::enum_name(state));
        break;
    }
}

void FrameGraph::SortGraph()
{
    enum class NodeStatus : uint8_t
    {
        eNotProcessed,
        eVisited,
        eAdded
    };

    std::vector<FrameGraphNodeHandle> reverseSortedNodes {};
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

    assert(_nodes.size() >= reverseSortedNodes.size() && "The amount of sorted nodes is not the same as the amount of nodes given to the frame graph, this should never happen");

    _sortedNodes.clear();

    for (int32_t i = reverseSortedNodes.size() - 1; i >= 0; --i)
    {
        _sortedNodes.push_back(reverseSortedNodes[i]);
    }
}

FrameGraphResourceHandle FrameGraph::CreateOutputResource(const FrameGraphResourceCreation& creation, FrameGraphNodeHandle producer)
{
    assert(!HasAnyFlags(creation.type, FrameGraphResourceType::eNone) && "FrameGraphResource must have a type.");

    const FrameGraphResourceHandle resourceHandle = _resources.size();
    FrameGraphResource& resource = _resources.emplace_back(std::variant<std::monostate, FrameGraphResourceInfo::StageBuffer, ResourceHandle<GPUImage>> {});
    resource.type = creation.type;
    resource.name = GetResourceName(creation.type, creation.info.resource);

    // If the same resource is found as an earlier output, we increase the version of the resource
    auto itr = _outputResourcesMap.find(resource.name);
    if (itr != _outputResourcesMap.end())
    {
        // Save the newest resource version for fast look up later
        _newestVersionedResourcesMap[resource.name] = resourceHandle;

        resource.version = _resources[itr->second].version + 1;
        resource.name += + "_v-" + std::to_string(resource.version);
    }

    if (!HasAnyFlags(creation.type, FrameGraphResourceType::eReference))
    {
        resource.info = creation.info;
        resource.output = resourceHandle;
        resource.producer = producer;

        _outputResourcesMap.emplace(resource.name, resourceHandle);
    }

    return resourceHandle;
}

FrameGraphResourceHandle FrameGraph::CreateInputResource(const FrameGraphResourceCreation& creation)
{
    assert(!HasAnyFlags(creation.type, FrameGraphResourceType::eNone) && "FrameGraphResource must have a type.");

    const FrameGraphResourceHandle resourceHandle = _resources.size();
    FrameGraphResource& resource = _resources.emplace_back(std::variant<std::monostate, FrameGraphResourceInfo::StageBuffer, ResourceHandle<GPUImage>> {});
    resource.type = creation.type;
    resource.name = GetResourceName(creation.type, creation.info.resource);

    // If the resource has multiple versions, find the newest one and use that one as input instead
    auto itr = _newestVersionedResourcesMap.find(resource.name);
    if (itr != _outputResourcesMap.end())
    {
        resource.name = itr->second;
    }

    return resourceHandle;
}

std::string FrameGraph::GetResourceName(FrameGraphResourceType type, const FrameGraphResourceInfo::Resource& resource)
{
    auto resources { _context->Resources() };

    if (HasAnyFlags(type, FrameGraphResourceType::eAttachment | FrameGraphResourceType::eTexture))
    {
        const GPUImage* image = resources->ImageResourceManager().Access(std::get<ResourceHandle<GPUImage>>(resource));
        return image->name;
    }

    if (HasAnyFlags(type, FrameGraphResourceType::eBuffer))
    {
        const Buffer* buffer = resources->BufferResourceManager().Access(std::get<FrameGraphResourceInfo::StageBuffer>(resource).handle);
        return buffer->name;
    }

    assert(false && "Unsupported resource type!");
    return "";
}