#include "vulkan_helper.hpp"

void util::VK_ASSERT(vk::Result result, std::string_view message)
{
    if (result == vk::Result::eSuccess)
        return;

    static std::string completeMessage {};
    completeMessage = "[] ";
    auto resultStr = magic_enum::enum_name(result);

    completeMessage.insert(1, resultStr);
    completeMessage.insert(completeMessage.size(), message);

    throw std::runtime_error(completeMessage.c_str());
}

void util::VK_ASSERT(VkResult result, std::string_view message)
{
    VK_ASSERT(vk::Result(result), message);
}

bool util::HasStencilComponent(vk::Format format)
{
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

std::optional<vk::Format> util::FindSupportedFormat(const vk::PhysicalDevice physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
    vk::FormatFeatureFlags features)
{
    for (vk::Format format : candidates)
    {
        vk::FormatProperties props;
        physicalDevice.getFormatProperties(format, &props);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
            return format;
        else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
            return format;
    }

    return std::nullopt;
}

void util::CreateBuffer(const VulkanBrain& brain, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::Buffer& buffer, bool mappable, VmaAllocation& allocation, VmaMemoryUsage memoryUsage, std::string_view name)
{
    vk::BufferCreateInfo bufferInfo {};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    bufferInfo.queueFamilyIndexCount = 1;
    bufferInfo.pQueueFamilyIndices = &brain.queueFamilyIndices.graphicsFamily.value();

    VmaAllocationCreateInfo allocationInfo {};
    allocationInfo.usage = memoryUsage;
    if (mappable)
        allocationInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    util::VK_ASSERT(vmaCreateBuffer(brain.vmaAllocator, reinterpret_cast<VkBufferCreateInfo*>(&bufferInfo), &allocationInfo, reinterpret_cast<VkBuffer*>(&buffer), &allocation, nullptr), "Failed creating buffer!");
    vmaSetAllocationName(brain.vmaAllocator, allocation, name.data());
    util::NameObject(buffer, name, brain.device, brain.dldi);
}

vk::CommandBuffer util::BeginSingleTimeCommands(const VulkanBrain& brain)
{
    vk::CommandBufferAllocateInfo allocateInfo {};
    allocateInfo.level = vk::CommandBufferLevel::ePrimary;
    allocateInfo.commandPool = brain.commandPool;
    allocateInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer;
    util::VK_ASSERT(brain.device.allocateCommandBuffers(&allocateInfo, &commandBuffer), "Failed allocating one time command buffer!");

    vk::CommandBufferBeginInfo beginInfo {};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    util::VK_ASSERT(commandBuffer.begin(&beginInfo), "Failed beginning one time command buffer!");

    return commandBuffer;
}

void util::EndSingleTimeCommands(const VulkanBrain& brain, vk::CommandBuffer commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    util::VK_ASSERT(brain.graphicsQueue.submit(1, &submitInfo, nullptr), "Failed submitting one time buffer to queue!");
    brain.graphicsQueue.waitIdle();

    brain.device.free(brain.commandPool, commandBuffer);
}

void util::CopyBuffer(vk::CommandBuffer commandBuffer, vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, uint32_t offset)
{
    vk::BufferCopy copyRegion {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = offset;
    copyRegion.size = size;
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
}

vk::UniqueSampler util::CreateSampler(const VulkanBrain& brain, vk::Filter min, vk::Filter mag, vk::SamplerAddressMode addressingMode, vk::SamplerMipmapMode mipmapMode, uint32_t mipLevels)
{
    vk::PhysicalDeviceProperties properties {};
    brain.physicalDevice.getProperties(&properties);

    vk::SamplerCreateInfo createInfo {};
    createInfo.magFilter = mag;
    createInfo.minFilter = min;
    createInfo.addressModeU = addressingMode;
    createInfo.addressModeV = addressingMode;
    createInfo.addressModeW = addressingMode;
    createInfo.anisotropyEnable = 1;
    createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    createInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    createInfo.unnormalizedCoordinates = 0;
    createInfo.compareEnable = 0;
    createInfo.compareOp = vk::CompareOp::eAlways;
    createInfo.mipmapMode = mipmapMode;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = static_cast<float>(mipLevels);
    // createInfo.compareEnable = VK_TRUE; // Enable depth comparison
    // createInfo.compareOp = vk::CompareOp::eLessOrEqual;

    return brain.device.createSamplerUnique(createInfo);
}

util::ImageLayoutTransitionState util::GetImageLayoutTransitionSourceState(vk::ImageLayout sourceLayout)
{
    static const std::unordered_map<vk::ImageLayout, ImageLayoutTransitionState> sourceStateMap = {
        { vk::ImageLayout::eUndefined,
            { .pipelineStage = vk::PipelineStageFlagBits2::eTopOfPipe,
                .accessFlags = vk::AccessFlags2 { 0 } } },
        { vk::ImageLayout::eTransferDstOptimal,
            { .pipelineStage = vk::PipelineStageFlagBits2::eTransfer,
                .accessFlags = vk::AccessFlagBits2::eTransferWrite } },
        { vk::ImageLayout::eTransferSrcOptimal,
            { .pipelineStage = vk::PipelineStageFlagBits2::eTransfer,
                .accessFlags = vk::AccessFlagBits2::eTransferWrite } },
        { vk::ImageLayout::eShaderReadOnlyOptimal,
            { .pipelineStage = vk::PipelineStageFlagBits2::eFragmentShader,
                .accessFlags = vk::AccessFlagBits2::eShaderRead } },
        { vk::ImageLayout::eColorAttachmentOptimal,
            { .pipelineStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                .accessFlags = vk::AccessFlagBits2::eColorAttachmentWrite } },
        { vk::ImageLayout::eDepthStencilAttachmentOptimal,
            { .pipelineStage = vk::PipelineStageFlagBits2::eLateFragmentTests,
                .accessFlags = vk::AccessFlagBits2::eDepthStencilAttachmentWrite } },
    };

    auto it = sourceStateMap.find(sourceLayout);

    if (it == sourceStateMap.end())
    {
        throw std::runtime_error("Unsupported source state for image layout transition!");
    }

    return it->second;
}

util::ImageLayoutTransitionState util::GetImageLayoutTransitionDestinationState(vk::ImageLayout destinationLayout)
{
    static const std::unordered_map<vk::ImageLayout, ImageLayoutTransitionState> destinationStateMap = {
        { vk::ImageLayout::eTransferDstOptimal,
            { .pipelineStage = vk::PipelineStageFlagBits2::eTransfer,
                .accessFlags = vk::AccessFlagBits2::eTransferWrite } },
        { vk::ImageLayout::eTransferSrcOptimal,
            { .pipelineStage = vk::PipelineStageFlagBits2::eTransfer,
                .accessFlags = vk::AccessFlagBits2::eTransferRead } },
        { vk::ImageLayout::eShaderReadOnlyOptimal,
            { .pipelineStage = vk::PipelineStageFlagBits2::eFragmentShader,
                .accessFlags = vk::AccessFlagBits2::eShaderRead } },
        { vk::ImageLayout::eColorAttachmentOptimal,
            { .pipelineStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                .accessFlags = vk::AccessFlagBits2::eColorAttachmentWrite } },
        { vk::ImageLayout::eDepthStencilAttachmentOptimal,
            { .pipelineStage = vk::PipelineStageFlagBits2::eEarlyFragmentTests,
                .accessFlags = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite } },
        { vk::ImageLayout::ePresentSrcKHR,
            { .pipelineStage = vk::PipelineStageFlagBits2::eBottomOfPipe,
                .accessFlags = vk::AccessFlags2 { 0 } } },
        { vk::ImageLayout::eDepthStencilReadOnlyOptimal,
            { .pipelineStage = vk::PipelineStageFlagBits2::eEarlyFragmentTests,
                .accessFlags = vk::AccessFlagBits2::eDepthStencilAttachmentRead } }
    };

    auto it = destinationStateMap.find(destinationLayout);

    if (it == destinationStateMap.end())
    {
        throw std::runtime_error("Unsupported destination state for image layout transition!");
    }

    return it->second;
}

void util::InitializeImageMemoryBarrier(vk::ImageMemoryBarrier2& barrier, vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t numLayers, uint32_t mipLevel, uint32_t mipCount, vk::ImageAspectFlagBits imageAspect)
{
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = imageAspect;
    barrier.subresourceRange.baseMipLevel = mipLevel;
    barrier.subresourceRange.levelCount = mipCount;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = numLayers;

    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
    {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        if (util::HasStencilComponent(format))
            barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
    }

    const ImageLayoutTransitionState sourceState = GetImageLayoutTransitionSourceState(oldLayout);
    const ImageLayoutTransitionState destinationState = GetImageLayoutTransitionDestinationState(newLayout);

    barrier.srcStageMask = sourceState.pipelineStage;
    barrier.srcAccessMask = sourceState.accessFlags;
    barrier.dstStageMask = destinationState.pipelineStage;
    barrier.dstAccessMask = destinationState.accessFlags;
}

void util::TransitionImageLayout(vk::CommandBuffer commandBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t numLayers, uint32_t mipLevel, uint32_t mipCount, vk::ImageAspectFlagBits imageAspect)
{
    vk::ImageMemoryBarrier2 barrier {};
    InitializeImageMemoryBarrier(barrier, image, format, oldLayout, newLayout, numLayers, mipLevel, mipCount, imageAspect);

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.setImageMemoryBarrierCount(1)
        .setPImageMemoryBarriers(&barrier);

    commandBuffer.pipelineBarrier2(dependencyInfo);
}

void util::CopyBufferToImage(vk::CommandBuffer commandBuffer, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
{
    vk::BufferImageCopy region {};
    region.bufferImageHeight = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = vk::Offset3D { 0, 0, 0 };
    region.imageExtent = vk::Extent3D { width, height, 1 };

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);
}

void util::BeginLabel(vk::Queue queue, std::string_view label, glm::vec3 color, const vk::DispatchLoaderDynamic dldi)
{
    if (!ENABLE_VALIDATION_LAYERS)
        return;
    vk::DebugUtilsLabelEXT labelExt {};
    memcpy(labelExt.color.data(), &color.r, sizeof(glm::vec3));
    labelExt.color[3] = 1.0f;
    labelExt.pLabelName = label.data();

    queue.beginDebugUtilsLabelEXT(&labelExt, dldi);
}

void util::EndLabel(vk::Queue queue, const vk::DispatchLoaderDynamic dldi)
{
    if (!ENABLE_VALIDATION_LAYERS)
        return;
    queue.endDebugUtilsLabelEXT(dldi);
}

void util::BeginLabel(vk::CommandBuffer commandBuffer, std::string_view label, glm::vec3 color, const vk::DispatchLoaderDynamic dldi)
{
    if (!ENABLE_VALIDATION_LAYERS)
        return;
    vk::DebugUtilsLabelEXT labelExt {};
    memcpy(labelExt.color.data(), &color.r, sizeof(glm::vec3));
    labelExt.color[3] = 1.0f;
    labelExt.pLabelName = label.data();

    commandBuffer.beginDebugUtilsLabelEXT(&labelExt, dldi);
}

void util::EndLabel(vk::CommandBuffer commandBuffer, const vk::DispatchLoaderDynamic dldi)
{
    if (!ENABLE_VALIDATION_LAYERS)
        return;
    commandBuffer.endDebugUtilsLabelEXT(dldi);
}

vk::ImageAspectFlags util::GetImageAspectFlags(vk::Format format)
{
    switch (format)
    {
    // Depth formats
    case vk::Format::eD16Unorm:
    case vk::Format::eX8D24UnormPack32:
    case vk::Format::eD32Sfloat:
        return vk::ImageAspectFlagBits::eDepth;

    // Depth-stencil formats
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
        return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

    // Color formats
    default:
        // Most other formats are color formats
        if (format >= vk::Format::eR4G4UnormPack8 && format <= vk::Format::eBc7UnormBlock)
        {
            return vk::ImageAspectFlagBits::eColor;
        }
        else
        {
            // Handle error or unsupported format case
            throw std::runtime_error("Unsupported format for aspect determination.");
        }
    }
}
