#include "single_time_commands.hpp"
#include "vulkan_helper.hpp"

SingleTimeCommands::SingleTimeCommands(const VulkanBrain& brain) :
    _brain(brain)
{
    vk::CommandBufferAllocateInfo allocateInfo{};
    allocateInfo.level = vk::CommandBufferLevel::ePrimary;
    allocateInfo.commandPool = brain.commandPool;
    allocateInfo.commandBufferCount = 1;

    util::VK_ASSERT(brain.device.allocateCommandBuffers(&allocateInfo, &_commandBuffer), "Failed allocating one time command buffer!");

    vk::FenceCreateInfo fenceInfo{};
    util::VK_ASSERT(_brain.device.createFence(&fenceInfo, nullptr, &_fence), "Failed creating single time command fence!");

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    util::VK_ASSERT(_commandBuffer.begin(&beginInfo), "Failed beginning one time command buffer!");
}

SingleTimeCommands::~SingleTimeCommands()
{
    Submit();
}

void SingleTimeCommands::Submit()
{
    if(_submitted)
        return;

    _commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffer;

    spdlog::info("Submitting single-time commands");
    util::VK_ASSERT(_brain.graphicsQueue.submit(1, &submitInfo, _fence), "Failed submitting one time buffer to queue!");
    util::VK_ASSERT(_brain.device.waitForFences(1, &_fence, VK_TRUE, std::numeric_limits<uint64_t>::max()), "Failed waiting for fence!");

    _brain.device.free(_brain.commandPool, _commandBuffer);
    _brain.device.destroy(_fence);

    assert(_stagingAllocations.size() == _stagingBuffers.size());
    for(size_t i = 0; i < _stagingBuffers.size(); ++i)
    {
        vmaDestroyBuffer(_brain.vmaAllocator, _stagingBuffers[i], _stagingAllocations[i]);
    }
    _submitted = true;
}

void SingleTimeCommands::CreateTextureImage(const Texture& texture, TextureHandle& textureHandle, bool generateMips)
{
    textureHandle.width = texture.width;
    textureHandle.height = texture.height;

    vk::DeviceSize imageSize = texture.width * texture.height * texture.numChannels;
    if(texture.isHDR)
        imageSize *= sizeof(float);

    vk::Buffer& stagingBuffer = _stagingBuffers.emplace_back();
    VmaAllocation& stagingBufferAllocation = _stagingAllocations.emplace_back();

    util::CreateBuffer(_brain, imageSize, vk::BufferUsageFlagBits::eTransferSrc, stagingBuffer, true, stagingBufferAllocation, VMA_MEMORY_USAGE_CPU_ONLY, "Texture staging buffer");

    vmaCopyMemoryToAllocation(_brain.vmaAllocator, texture.data.data(), stagingBufferAllocation, 0, imageSize);

    util::CreateImage(_brain.vmaAllocator, texture.width, texture.height, texture.GetFormat(),
                      vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled,
                      textureHandle.image, textureHandle.imageAllocation, "Texture image", generateMips, VMA_MEMORY_USAGE_GPU_ONLY);


    vk::ImageLayout oldLayout = vk::ImageLayout::eTransferDstOptimal;

    util::TransitionImageLayout(_commandBuffer, textureHandle.image, texture.GetFormat(), vk::ImageLayout::eUndefined, oldLayout);

    util::CopyBufferToImage(_commandBuffer, stagingBuffer, textureHandle.image, texture.width, texture.height);

    uint32_t mipCount = 1;
    if(generateMips)
    {
        util::TransitionImageLayout(_commandBuffer, textureHandle.image, texture.GetFormat(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, 1, 0, 1);

        mipCount = static_cast<uint32_t>(floor(log2(std::max(texture.width, texture.height))) + 1);
        for(uint32_t i = 1; i < mipCount; ++i)
        {
            vk::ImageBlit blit{};
            blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.srcSubresource.layerCount = 1;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcOffsets[1].x = texture.width >> (i - 1);
            blit.srcOffsets[1].y = texture.height >> (i - 1);
            blit.srcOffsets[1].z = 1;

            blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.dstSubresource.layerCount = 1;
            blit.dstSubresource.mipLevel = i;
            blit.dstOffsets[1].x = texture.width >> i;
            blit.dstOffsets[1].y = texture.height >> i;
            blit.dstOffsets[1].z = 1;

            util::TransitionImageLayout(_commandBuffer, textureHandle.image, texture.GetFormat(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, i);

            _commandBuffer.blitImage(textureHandle.image, vk::ImageLayout::eTransferSrcOptimal, textureHandle.image, vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

            util::TransitionImageLayout(_commandBuffer, textureHandle.image, texture.GetFormat(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, 1, i);
        }
        oldLayout = vk::ImageLayout::eTransferSrcOptimal;
    }

    util::TransitionImageLayout(_commandBuffer, textureHandle.image, texture.GetFormat(), oldLayout, vk::ImageLayout::eShaderReadOnlyOptimal, 1, 0, mipCount);

    textureHandle.imageView = util::CreateImageView(_brain.device, textureHandle.image, texture.GetFormat(), vk::ImageAspectFlagBits::eColor, 0, mipCount);
}

void SingleTimeCommands::CreateLocalBuffer(const std::byte* vec, uint32_t count, vk::Buffer& buffer,
                                           VmaAllocation& allocation, vk::BufferUsageFlags usage, std::string_view name)
{
    vk::DeviceSize bufferSize = count;

    vk::Buffer& stagingBuffer = _stagingBuffers.emplace_back();
    VmaAllocation& stagingBufferAllocation = _stagingAllocations.emplace_back();
    util::CreateBuffer(_brain, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, stagingBuffer, true, stagingBufferAllocation, VMA_MEMORY_USAGE_CPU_ONLY, "Staging buffer");

    vmaCopyMemoryToAllocation(_brain.vmaAllocator, vec, stagingBufferAllocation, 0, bufferSize);

    util::CreateBuffer(_brain, bufferSize, vk::BufferUsageFlagBits::eTransferDst | usage, buffer, false, allocation, VMA_MEMORY_USAGE_GPU_ONLY, name.data());

    util::CopyBuffer(_commandBuffer, stagingBuffer, buffer, bufferSize);
}
