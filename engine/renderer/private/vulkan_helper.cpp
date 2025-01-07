#include "vulkan_helper.hpp"

#include "tracy/Tracy.hpp"
#include "vk_mem_alloc.h"
#include "vulkan_include.hpp"

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

void util::VK_ASSERT(SpvReflectResult result, std::string_view message)
{
    if (result == SPV_REFLECT_RESULT_SUCCESS)
        return;

    static std::string completeMessage {};
    completeMessage = "[] ";
    auto resultStr = magic_enum::enum_name(result);

    completeMessage.insert(1, resultStr);
    completeMessage.insert(completeMessage.size(), message);

    throw std::runtime_error(completeMessage.c_str());
}

std::unordered_multimap<VmaAllocation, vk::MemoryPropertyFlagBits> allocationMemoryTypeMap {};

VkResult util::vmaCreateBuffer(VmaAllocator allocator, const VkBufferCreateInfo* pBufferCreateInfo, const VmaAllocationCreateInfo* pAllocationCreateInfo, VkBuffer* pBuffer, VmaAllocation* pAllocation, VmaAllocationInfo* pAllocationInfo)
{
#ifdef TRACY_ENABLE
    VmaAllocationInfo allocInfo;
    if (pAllocationInfo == nullptr)
    {
        pAllocationInfo = &allocInfo;
    }
#endif

    auto result = ::vmaCreateBuffer(allocator, pBufferCreateInfo, pAllocationCreateInfo, pBuffer, pAllocation, pAllocationInfo);

#ifdef TRACY_ENABLE
    if (result == VK_SUCCESS)
    {
        TracyAllocN(*pAllocation, pAllocationInfo->size, "GPU Memory usage");
        TracyAllocN(*pAllocation, pAllocationInfo->size, "GPU Memory usage (Buffer)");
    }
#endif

    return result;
}

void util::vmaDestroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation)
{
#ifdef TRACY_ENABLE
    TracyFreeN(allocation, "GPU Memory usage");
    TracyFreeN(allocation, "GPU Memory usage (Buffer)");
#endif

    ::vmaDestroyBuffer(allocator, buffer, allocation);
}

VkResult util::vmaCreateImage(VmaAllocator allocator, const VkImageCreateInfo* pImageCreateInfo, const VmaAllocationCreateInfo* pAllocationCreateInfo, VkImage* pImage, VmaAllocation* pAllocation, VmaAllocationInfo* pAllocationInfo)
{
#ifdef TRACY_ENABLE
    VmaAllocationInfo allocInfo;
    if (pAllocationInfo == nullptr)
    {
        pAllocationInfo = &allocInfo;
    }
#endif

    auto result = ::vmaCreateImage(allocator, pImageCreateInfo, pAllocationCreateInfo, pImage, pAllocation, pAllocationInfo);

#ifdef TRACY_ENABLE
    if (result == VK_SUCCESS)
    {
        TracyAllocN(*pAllocation, pAllocationInfo->size, "GPU Memory usage");
        TracyAllocN(*pAllocation, pAllocationInfo->size, "GPU Memory usage (Image)");
    }
#endif

    return result;
}

void util::vmaDestroyImage(VmaAllocator allocator, VkImage image, VmaAllocation allocation)
{
#ifdef TRACY_ENABLE
    TracyFreeN(allocation, "GPU Memory usage");
    TracyFreeN(allocation, "GPU Memory usage (Image)");
#endif

    ::vmaDestroyImage(allocator, image, allocation);
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

void util::CreateBuffer(std::shared_ptr<VulkanContext> context, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::Buffer& buffer, bool mappable, VmaAllocation& allocation, VmaMemoryUsage memoryUsage, std::string_view name)
{
    vk::BufferCreateInfo bufferInfo {};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    bufferInfo.queueFamilyIndexCount = 1;
    bufferInfo.pQueueFamilyIndices = &context->QueueFamilies().graphicsFamily.value();

    VmaAllocationCreateInfo allocationInfo {};
    allocationInfo.usage = memoryUsage;
    if (mappable)
        allocationInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    util::VK_ASSERT(util::vmaCreateBuffer(context->MemoryAllocator(), reinterpret_cast<VkBufferCreateInfo*>(&bufferInfo), &allocationInfo, reinterpret_cast<VkBuffer*>(&buffer), &allocation, nullptr), "Failed creating buffer!");
    vmaSetAllocationName(context->MemoryAllocator(), allocation, name.data());
    util::NameObject(buffer, name, context);
}

vk::CommandBuffer util::BeginSingleTimeCommands(std::shared_ptr<VulkanContext> context)
{
    vk::CommandBufferAllocateInfo allocateInfo {};
    allocateInfo.level = vk::CommandBufferLevel::ePrimary;
    allocateInfo.commandPool = context->CommandPool();
    allocateInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer;
    util::VK_ASSERT(context->Device().allocateCommandBuffers(&allocateInfo, &commandBuffer), "Failed allocating one time command buffer!");

    vk::CommandBufferBeginInfo beginInfo {};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    util::VK_ASSERT(commandBuffer.begin(&beginInfo), "Failed beginning one time command buffer!");

    return commandBuffer;
}

void util::EndSingleTimeCommands(std::shared_ptr<VulkanContext> context, vk::CommandBuffer commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    util::VK_ASSERT(context->GraphicsQueue().submit(1, &submitInfo, nullptr), "Failed submitting one time buffer to queue!");
    context->GraphicsQueue().waitIdle();

    context->Device().free(context->CommandPool(), commandBuffer);
}

void util::CopyBuffer(vk::CommandBuffer commandBuffer, vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, uint32_t offset)
{
    vk::BufferCopy copyRegion {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = offset;
    copyRegion.size = size;
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
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
void util::CopyImageToImage(vk::CommandBuffer commandBuffer, vk::Image srcImage, vk::Image dstImage, vk::Extent2D srcSize, vk::Extent2D dstSize)
{
    vk::ImageBlit2 region {};
    region.sType = vk::StructureType::eImageBlit2;
    region.pNext = nullptr;

    region.srcOffsets[1].x = srcSize.width;
    region.srcOffsets[1].y = srcSize.height;
    region.srcOffsets[1].z = 1;

    region.dstOffsets[1].x = dstSize.width;
    region.dstOffsets[1].y = dstSize.height;
    region.dstOffsets[1].z = 1;

    region.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = 1;
    region.srcSubresource.mipLevel = 0;
    region.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = 1;
    region.dstSubresource.mipLevel = 0;

    vk::BlitImageInfo2 blitInfo {};
    blitInfo.sType = vk::StructureType::eBlitImageInfo2;
    blitInfo.pNext = nullptr;
    blitInfo.dstImage = dstImage;
    blitInfo.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
    blitInfo.srcImage = srcImage;
    blitInfo.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
    blitInfo.filter = vk::Filter::eLinear;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &region;

    commandBuffer.blitImage2(&blitInfo);
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

// Returns the size in bytes of the provided vk::Format.
// As this is only intended for vertex attribute formats, not all VkFormats are
// supported.
uint32_t util::FormatSize(vk::Format format)
{
    uint32_t result = 0;
    switch (static_cast<VkFormat>(format))
    {
    case VK_FORMAT_UNDEFINED:
        result = 0;
        break;
    case VK_FORMAT_R4G4_UNORM_PACK8:
        result = 1;
        break;
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
        result = 2;
        break;
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        result = 2;
        break;
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
        result = 2;
        break;
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
        result = 2;
        break;
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
        result = 2;
        break;
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
        result = 2;
        break;
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        result = 2;
        break;
    case VK_FORMAT_R8_UNORM:
        result = 1;
        break;
    case VK_FORMAT_R8_SNORM:
        result = 1;
        break;
    case VK_FORMAT_R8_USCALED:
        result = 1;
        break;
    case VK_FORMAT_R8_SSCALED:
        result = 1;
        break;
    case VK_FORMAT_R8_UINT:
        result = 1;
        break;
    case VK_FORMAT_R8_SINT:
        result = 1;
        break;
    case VK_FORMAT_R8_SRGB:
        result = 1;
        break;
    case VK_FORMAT_R8G8_UNORM:
        result = 2;
        break;
    case VK_FORMAT_R8G8_SNORM:
        result = 2;
        break;
    case VK_FORMAT_R8G8_USCALED:
        result = 2;
        break;
    case VK_FORMAT_R8G8_SSCALED:
        result = 2;
        break;
    case VK_FORMAT_R8G8_UINT:
        result = 2;
        break;
    case VK_FORMAT_R8G8_SINT:
        result = 2;
        break;
    case VK_FORMAT_R8G8_SRGB:
        result = 2;
        break;
    case VK_FORMAT_R8G8B8_UNORM:
        result = 3;
        break;
    case VK_FORMAT_R8G8B8_SNORM:
        result = 3;
        break;
    case VK_FORMAT_R8G8B8_USCALED:
        result = 3;
        break;
    case VK_FORMAT_R8G8B8_SSCALED:
        result = 3;
        break;
    case VK_FORMAT_R8G8B8_UINT:
        result = 3;
        break;
    case VK_FORMAT_R8G8B8_SINT:
        result = 3;
        break;
    case VK_FORMAT_R8G8B8_SRGB:
        result = 3;
        break;
    case VK_FORMAT_B8G8R8_UNORM:
        result = 3;
        break;
    case VK_FORMAT_B8G8R8_SNORM:
        result = 3;
        break;
    case VK_FORMAT_B8G8R8_USCALED:
        result = 3;
        break;
    case VK_FORMAT_B8G8R8_SSCALED:
        result = 3;
        break;
    case VK_FORMAT_B8G8R8_UINT:
        result = 3;
        break;
    case VK_FORMAT_B8G8R8_SINT:
        result = 3;
        break;
    case VK_FORMAT_B8G8R8_SRGB:
        result = 3;
        break;
    case VK_FORMAT_R8G8B8A8_UNORM:
        result = 4;
        break;
    case VK_FORMAT_R8G8B8A8_SNORM:
        result = 4;
        break;
    case VK_FORMAT_R8G8B8A8_USCALED:
        result = 4;
        break;
    case VK_FORMAT_R8G8B8A8_SSCALED:
        result = 4;
        break;
    case VK_FORMAT_R8G8B8A8_UINT:
        result = 4;
        break;
    case VK_FORMAT_R8G8B8A8_SINT:
        result = 4;
        break;
    case VK_FORMAT_R8G8B8A8_SRGB:
        result = 4;
        break;
    case VK_FORMAT_B8G8R8A8_UNORM:
        result = 4;
        break;
    case VK_FORMAT_B8G8R8A8_SNORM:
        result = 4;
        break;
    case VK_FORMAT_B8G8R8A8_USCALED:
        result = 4;
        break;
    case VK_FORMAT_B8G8R8A8_SSCALED:
        result = 4;
        break;
    case VK_FORMAT_B8G8R8A8_UINT:
        result = 4;
        break;
    case VK_FORMAT_B8G8R8A8_SINT:
        result = 4;
        break;
    case VK_FORMAT_B8G8R8A8_SRGB:
        result = 4;
        break;
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        result = 4;
        break;
    case VK_FORMAT_R16_UNORM:
        result = 2;
        break;
    case VK_FORMAT_R16_SNORM:
        result = 2;
        break;
    case VK_FORMAT_R16_USCALED:
        result = 2;
        break;
    case VK_FORMAT_R16_SSCALED:
        result = 2;
        break;
    case VK_FORMAT_R16_UINT:
        result = 2;
        break;
    case VK_FORMAT_R16_SINT:
        result = 2;
        break;
    case VK_FORMAT_R16_SFLOAT:
        result = 2;
        break;
    case VK_FORMAT_R16G16_UNORM:
        result = 4;
        break;
    case VK_FORMAT_R16G16_SNORM:
        result = 4;
        break;
    case VK_FORMAT_R16G16_USCALED:
        result = 4;
        break;
    case VK_FORMAT_R16G16_SSCALED:
        result = 4;
        break;
    case VK_FORMAT_R16G16_UINT:
        result = 4;
        break;
    case VK_FORMAT_R16G16_SINT:
        result = 4;
        break;
    case VK_FORMAT_R16G16_SFLOAT:
        result = 4;
        break;
    case VK_FORMAT_R16G16B16_UNORM:
        result = 6;
        break;
    case VK_FORMAT_R16G16B16_SNORM:
        result = 6;
        break;
    case VK_FORMAT_R16G16B16_USCALED:
        result = 6;
        break;
    case VK_FORMAT_R16G16B16_SSCALED:
        result = 6;
        break;
    case VK_FORMAT_R16G16B16_UINT:
        result = 6;
        break;
    case VK_FORMAT_R16G16B16_SINT:
        result = 6;
        break;
    case VK_FORMAT_R16G16B16_SFLOAT:
        result = 6;
        break;
    case VK_FORMAT_R16G16B16A16_UNORM:
        result = 8;
        break;
    case VK_FORMAT_R16G16B16A16_SNORM:
        result = 8;
        break;
    case VK_FORMAT_R16G16B16A16_USCALED:
        result = 8;
        break;
    case VK_FORMAT_R16G16B16A16_SSCALED:
        result = 8;
        break;
    case VK_FORMAT_R16G16B16A16_UINT:
        result = 8;
        break;
    case VK_FORMAT_R16G16B16A16_SINT:
        result = 8;
        break;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        result = 8;
        break;
    case VK_FORMAT_R32_UINT:
        result = 4;
        break;
    case VK_FORMAT_R32_SINT:
        result = 4;
        break;
    case VK_FORMAT_R32_SFLOAT:
        result = 4;
        break;
    case VK_FORMAT_R32G32_UINT:
        result = 8;
        break;
    case VK_FORMAT_R32G32_SINT:
        result = 8;
        break;
    case VK_FORMAT_R32G32_SFLOAT:
        result = 8;
        break;
    case VK_FORMAT_R32G32B32_UINT:
        result = 12;
        break;
    case VK_FORMAT_R32G32B32_SINT:
        result = 12;
        break;
    case VK_FORMAT_R32G32B32_SFLOAT:
        result = 12;
        break;
    case VK_FORMAT_R32G32B32A32_UINT:
        result = 16;
        break;
    case VK_FORMAT_R32G32B32A32_SINT:
        result = 16;
        break;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        result = 16;
        break;
    case VK_FORMAT_R64_UINT:
        result = 8;
        break;
    case VK_FORMAT_R64_SINT:
        result = 8;
        break;
    case VK_FORMAT_R64_SFLOAT:
        result = 8;
        break;
    case VK_FORMAT_R64G64_UINT:
        result = 16;
        break;
    case VK_FORMAT_R64G64_SINT:
        result = 16;
        break;
    case VK_FORMAT_R64G64_SFLOAT:
        result = 16;
        break;
    case VK_FORMAT_R64G64B64_UINT:
        result = 24;
        break;
    case VK_FORMAT_R64G64B64_SINT:
        result = 24;
        break;
    case VK_FORMAT_R64G64B64_SFLOAT:
        result = 24;
        break;
    case VK_FORMAT_R64G64B64A64_UINT:
        result = 32;
        break;
    case VK_FORMAT_R64G64B64A64_SINT:
        result = 32;
        break;
    case VK_FORMAT_R64G64B64A64_SFLOAT:
        result = 32;
        break;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        result = 4;
        break;
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
        result = 4;
        break;

    default:
        break;
    }
    return result;
}
