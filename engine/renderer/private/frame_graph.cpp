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

FrameGraphNodeCreation& FrameGraphNodeCreation::AddOutput(ResourceHandle<GPUImage> image, FrameGraphResourceType type)
{
    FrameGraphResourceCreation& creation = outputs.emplace_back(image);
    creation.type = type;

    return *this;
}

FrameGraphNodeCreation& FrameGraphNodeCreation::AddOutput(ResourceHandle<Buffer> buffer, FrameGraphResourceType type, vk::PipelineStageFlags2 stageUsage)
{
    FrameGraphResourceCreation& creation = outputs.emplace_back(FrameGraphResourceInfo::StageBuffer { .handle = buffer, .stageUsage = stageUsage });
    creation.type = type;

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

    // Describes if an output has already been used as an input
    std::unordered_map<FrameGraphResourceHandle, bool> outputResourceStates {};

    for (const FrameGraphNodeHandle nodeHandle : _sortedNodes)
    {
        FrameGraphNode& node = _nodes[nodeHandle];

        node.imageMemoryBarriers.clear();
        node.bufferMemoryBarriers.clear();

        // Handle input memory barriers
        for (const FrameGraphResourceHandle inputHandle : node.inputs)
        {
            const FrameGraphResource& resource = _resources[inputHandle];

            // If the resource was already used as an input, we know it has to be in the correct state to be used as an input again,
            // as switching resource types is not possible in the inputs. So no memory barrier is needed and we can skip to the next resource
            if (outputResourceStates[resource.output])
            {
                continue;
            }

            outputResourceStates[resource.output] = true;

            if (resource.type == FrameGraphResourceType::eTexture)
            {
                const GPUImage* texture = resources->ImageResourceManager().Access(std::get<ResourceHandle<GPUImage>>(resource.info.resource));
                vk::ImageMemoryBarrier2& barrier = node.imageMemoryBarriers.emplace_back();

                if (texture->flags & vk::ImageUsageFlagBits::eDepthStencilAttachment)
                {
                    util::InitializeImageMemoryBarrier(barrier, texture->image, texture->format,
                        vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                        texture->layers, 0, texture->mips, vk::ImageAspectFlagBits::eDepth);
                }
                else
                {
                    util::InitializeImageMemoryBarrier(barrier, texture->image, texture->format,
                        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                        texture->layers, 0, texture->mips, vk::ImageAspectFlagBits::eColor);
                }
            }
            else if (resource.type == FrameGraphResourceType::eBuffer)
            {
                auto stageBuffer = std::get<FrameGraphResourceInfo::StageBuffer>(resource.info.resource);
                const Buffer* buffer = resources->BufferResourceManager().Access(stageBuffer.handle);
                vk::BufferMemoryBarrier2& barrier = node.bufferMemoryBarriers.emplace_back();

                // Get the buffer created before here and create barrier based on its stage usage
                const FrameGraphResourceHandle outputResourceHandle = _outputResourcesMap[resource.name];
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
            }
        }

        // Handle output memory barriers
        for (const FrameGraphResourceHandle outputHandle : node.outputs)
        {
            const FrameGraphResource& resource = _resources[outputHandle];
            outputResourceStates[outputHandle] = false;

            if (resource.type == FrameGraphResourceType::eAttachment)
            {
                const GPUImage* attachment = resources->ImageResourceManager().Access(std::get<ResourceHandle<GPUImage>>(resource.info.resource));
                vk::ImageMemoryBarrier2& barrier = node.imageMemoryBarriers.emplace_back();

                if (attachment->flags & vk::ImageUsageFlagBits::eDepthStencilAttachment)
                {
                    util::InitializeImageMemoryBarrier(barrier, attachment->image, attachment->format,
                        vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal,
                        attachment->layers, 0, attachment->mips, vk::ImageAspectFlagBits::eDepth);
                }
                else
                {
                    util::InitializeImageMemoryBarrier(barrier, attachment->image, attachment->format,
                        vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
                        attachment->layers, 0, attachment->mips, vk::ImageAspectFlagBits::eColor);
                }
            }
        }

        // Compile all barriers into dependency info to run them in a batch
        node.dependencyInfo.setImageMemoryBarrierCount(node.imageMemoryBarriers.size())
            .setPImageMemoryBarriers(node.imageMemoryBarriers.data())
            .setBufferMemoryBarrierCount(node.bufferMemoryBarriers.size())
            .setPBufferMemoryBarriers(node.bufferMemoryBarriers.data());
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
    resource.name = GetResourceName(creation);

    if (!HasAnyFlags(creation.type, FrameGraphResourceType::eReference))
    {
        assert(_outputResourcesMap.find(GetResourceName(creation)) == _outputResourcesMap.end() && "Multiple nodes produce the same resource, which is not possible. If you want to change the order of nodes, please use the eReference resource type to reference resources produced by multiple nodes.");

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
    resource.name = GetResourceName(creation);

    return resourceHandle;
}

std::string FrameGraph::GetResourceName(const FrameGraphResourceCreation& creation)
{
    auto resources { _context->Resources() };

    if (HasAnyFlags(creation.type, FrameGraphResourceType::eAttachment | FrameGraphResourceType::eTexture))
    {
        const GPUImage* image = resources->ImageResourceManager().Access(std::get<ResourceHandle<GPUImage>>(creation.info.resource));
        return image->name;
    }

    if (HasAnyFlags(creation.type, FrameGraphResourceType::eBuffer))
    {
        const Buffer* buffer = resources->BufferResourceManager().Access(std::get<FrameGraphResourceInfo::StageBuffer>(creation.info.resource).handle);
        return buffer->name;
    }

    assert(false && "Unsupported resource type!");
    return "";
}