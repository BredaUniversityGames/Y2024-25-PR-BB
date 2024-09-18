#include "single_time_commands.hpp"
#include "include.hpp"
#include "vulkan_helper.hpp"
#include "vulkan_brain.hpp"

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
