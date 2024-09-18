#include "image_resource_manager.hpp"
#include "vulkan_brain.hpp"
#include "include.hpp"
#include "vulkan_helper.hpp"

ImageResourceManager::ImageResourceManager(const VulkanBrain &brain) : _brain(brain)
{

}

ResourceHandle<Image> ImageResourceManager::Create(const ImageCreation &creation)
{
    Image imageResource;

    imageResource.width = creation.width;
    imageResource.height = creation.height;
    imageResource.depth = creation.depth;
    imageResource.layers = creation.layers;
    imageResource.flags = creation.flags;
    imageResource.type = creation.type;
    imageResource.format = creation.format;
    imageResource.mips = std::min(creation.mips, static_cast<uint8_t>(floor(log2(std::max(imageResource.width, imageResource.height))) + 1));
    imageResource.name = creation.name;
    imageResource.isHDR = creation.isHDR;

    vk::ImageCreateInfo imageCreateInfo{};
    imageCreateInfo.imageType = creation.type;
    imageCreateInfo.extent.width = creation.width;
    imageCreateInfo.extent.height = creation.height;
    imageCreateInfo.extent.depth = creation.depth;
    imageCreateInfo.mipLevels = creation.mips;
    imageCreateInfo.arrayLayers = creation.layers;
    imageCreateInfo.format = creation.format;
    imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imageCreateInfo.samples = vk::SampleCountFlagBits::e1;

    imageCreateInfo.usage = creation.flags;

    if(creation.initialData)
        imageCreateInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    vmaCreateImage(_brain.vmaAllocator, (VkImageCreateInfo*)&imageCreateInfo, &allocCreateInfo, reinterpret_cast<VkImage*>(&imageResource.image), &imageResource.allocation, nullptr);
    std::string allocName = creation.name + " texture allocation";
    vmaSetAllocationName(_brain.vmaAllocator, imageResource.allocation, allocName.c_str());

    vk::ImageViewCreateInfo viewCreateInfo{};
    viewCreateInfo.image = imageResource.image;
    viewCreateInfo.viewType = static_cast<vk::ImageViewType>(creation.type);
    viewCreateInfo.format = creation.format;
    viewCreateInfo.subresourceRange.aspectMask = util::GetImageAspectFlags(imageResource.format);
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = creation.mips;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;

    for (size_t i = 0; i < creation.layers; ++i)
    {
        viewCreateInfo.subresourceRange.baseArrayLayer = i;
        vk::ImageView imageView;
        util::VK_ASSERT(_brain.device.createImageView(&viewCreateInfo, nullptr, &imageView), "Failed creating image view!");
        imageResource.views.emplace_back(imageView);
    }

    if(creation.initialData)
    {
        vk::DeviceSize imageSize = imageResource.width * imageResource.height * imageResource.depth * 4;
        if (imageResource.isHDR)
            imageSize *= sizeof(float);

        vk::Buffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;

        util::CreateBuffer(_brain, imageSize, vk::BufferUsageFlagBits::eTransferSrc, stagingBuffer, true, stagingBufferAllocation, VMA_MEMORY_USAGE_CPU_ONLY, "Texture staging buffer");

        vmaCopyMemoryToAllocation(_brain.vmaAllocator, creation.initialData, stagingBufferAllocation, 0, imageSize);

        vk::ImageLayout oldLayout = vk::ImageLayout::eTransferDstOptimal;

        vk::CommandBuffer commandBuffer = util::BeginSingleTimeCommands(_brain);

        util::TransitionImageLayout(commandBuffer, imageResource.image, imageResource.format, vk::ImageLayout::eUndefined, oldLayout);

        util::CopyBufferToImage(commandBuffer, stagingBuffer, imageResource.image, imageResource.width, imageResource.height);

        if (creation.mips > 1)
        {
            util::TransitionImageLayout(commandBuffer, imageResource.image, imageResource.format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, 1, 0, 1);

            for (uint32_t i = 1; i < creation.mips; ++i)
            {
                vk::ImageBlit blit{};
                blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                blit.srcSubresource.layerCount = 1;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcOffsets[1].x = imageResource.width >> (i - 1);
                blit.srcOffsets[1].y = imageResource.height >> (i - 1);
                blit.srcOffsets[1].z = 1;

                blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                blit.dstSubresource.layerCount = 1;
                blit.dstSubresource.mipLevel = i;
                blit.dstOffsets[1].x = imageResource.width >> i;
                blit.dstOffsets[1].y = imageResource.height >> i;
                blit.dstOffsets[1].z = 1;

                util::TransitionImageLayout(commandBuffer, imageResource.image, imageResource.format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, i);

                commandBuffer.blitImage(imageResource.image, vk::ImageLayout::eTransferSrcOptimal, imageResource.image, vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

                util::TransitionImageLayout(commandBuffer, imageResource.image, imageResource.format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, 1, i);
            }
            oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        }

        util::TransitionImageLayout(commandBuffer, imageResource.image, imageResource.format, oldLayout, vk::ImageLayout::eShaderReadOnlyOptimal, 1, 0, imageResource.mips);

        util::EndSingleTimeCommands(_brain, commandBuffer);

        vmaDestroyBuffer(_brain.vmaAllocator, stagingBuffer, stagingBufferAllocation);
    }

    return ResourceManager::Create(imageResource);
}

void ImageResourceManager::Destroy(ResourceHandle<Image> handle)
{
    if(IsValid(handle))
    {
        const Image* image = Access(handle);
        vmaDestroyImage(_brain.vmaAllocator, image->image, image->allocation);
        for(auto& view : image->views)
            _brain.device.destroy(view);

        ResourceManager::Destroy(handle);
    }
}
