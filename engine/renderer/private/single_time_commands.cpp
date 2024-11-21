#include "single_time_commands.hpp"
#include "graphics_context.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

SingleTimeCommands::SingleTimeCommands(const std::shared_ptr<GraphicsContext>& context)
    : _context(context)
{
    auto vkContext { _context->VulkanContext() };

    vk::CommandBufferAllocateInfo allocateInfo {};
    allocateInfo.level = vk::CommandBufferLevel::ePrimary;
    allocateInfo.commandPool = vkContext->CommandPool();
    allocateInfo.commandBufferCount = 1;

    util::VK_ASSERT(_context->VulkanContext()->Device().allocateCommandBuffers(&allocateInfo, &_commandBuffer), "Failed allocating one time command buffer!");

    vk::FenceCreateInfo fenceInfo {};
    util::VK_ASSERT(vkContext->Device().createFence(&fenceInfo, nullptr, &_fence), "Failed creating single time command fence!");

    vk::CommandBufferBeginInfo beginInfo {};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    util::VK_ASSERT(_commandBuffer.begin(&beginInfo), "Failed beginning one time command buffer!");
}

SingleTimeCommands::~SingleTimeCommands()
{
    Submit();
}

void SingleTimeCommands::Submit()
{
    if (_submitted)
        return;

    auto vkContext { _context->VulkanContext() };

    _commandBuffer.end();

    vk::SubmitInfo submitInfo {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffer;

    util::VK_ASSERT(vkContext->GraphicsQueue().submit(1, &submitInfo, _fence), "Failed submitting one time buffer to queue!");
    util::VK_ASSERT(vkContext->Device().waitForFences(1, &_fence, VK_TRUE, std::numeric_limits<uint64_t>::max()), "Failed waiting for fence!");

    vkContext->Device().free(vkContext->CommandPool(), _commandBuffer);
    vkContext->Device().destroy(_fence);

    assert(_stagingAllocations.size() == _stagingBuffers.size());
    for (size_t i = 0; i < _stagingBuffers.size(); ++i)
    {
        vmaDestroyBuffer(vkContext->MemoryAllocator(), _stagingBuffers[i], _stagingAllocations[i]);
    }
    _submitted = true;
}

void SingleTimeCommands::CreateLocalBuffer(const std::byte* vec, uint32_t count, vk::Buffer& buffer,
    VmaAllocation& allocation, vk::BufferUsageFlags usage, std::string_view name)
{
    auto vkContext { _context->VulkanContext() };

    vk::DeviceSize bufferSize = count;

    vk::Buffer& stagingBuffer = _stagingBuffers.emplace_back();
    VmaAllocation& stagingBufferAllocation = _stagingAllocations.emplace_back();
    util::CreateBuffer(vkContext, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, stagingBuffer, true, stagingBufferAllocation, VMA_MEMORY_USAGE_CPU_ONLY, "Staging buffer");

    vmaCopyMemoryToAllocation(vkContext->MemoryAllocator(), vec, stagingBufferAllocation, 0, bufferSize);

    util::CreateBuffer(vkContext, bufferSize, vk::BufferUsageFlagBits::eTransferDst | usage, buffer, false, allocation, VMA_MEMORY_USAGE_GPU_ONLY, name.data());

    util::CopyBuffer(_commandBuffer, stagingBuffer, buffer, bufferSize);
}

void SingleTimeCommands::CopyIntoLocalBuffer(const std::byte* vec, uint32_t count, uint32_t offset, vk::Buffer buffer)
{
    auto vkContext { _context->VulkanContext() };

    vk::DeviceSize bufferSize = count;

    vk::Buffer& stagingBuffer = _stagingBuffers.emplace_back();
    VmaAllocation& stagingBufferAllocation = _stagingAllocations.emplace_back();
    util::CreateBuffer(vkContext, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, stagingBuffer, true, stagingBufferAllocation, VMA_MEMORY_USAGE_CPU_ONLY, "Staging buffer");

    vmaCopyMemoryToAllocation(vkContext->MemoryAllocator(), vec, stagingBufferAllocation, 0, bufferSize);

    util::CopyBuffer(_commandBuffer, stagingBuffer, buffer, bufferSize, offset);
}