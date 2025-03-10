#include "memory/batch_allocator.hpp"

#include "vulkan_helper.hpp"

BatchAllocator::BatchAllocator(std::shared_ptr<GraphicsContext> context, const BufferCreation& sourceBufferParams)
    : _context(context)
{
    auto resources { _context->Resources() };
    _buffer = resources->BufferResourceManager().Create(sourceBufferParams);

    VmaVirtualBlockCreateInfo blockCreateInfo = {};
    blockCreateInfo.size = sourceBufferParams.size;

    VkResult res = vmaCreateVirtualBlock(&blockCreateInfo, &_subAllocator);
    util::VK_ASSERT(res, "Failed to create vma virtual block");
}

BatchAllocator::~BatchAllocator()
{
    vmaDestroyVirtualBlock(_subAllocator);
}

BatchAllocator::BatchAllocation BatchAllocator::Allocate(SingleTimeCommands& commands, const std::byte* data, size_t size)
{
    VmaVirtualAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.size = size;

    VkDeviceSize offset {};
    VmaVirtualAllocation alloc {};
    auto result = vmaVirtualAllocate(_subAllocator, &allocCreateInfo, &alloc, &offset);

    if (result == VK_SUCCESS)
    {
        auto resources { _context->Resources() };
        const Buffer* buffer = resources->BufferResourceManager().Access(_buffer);
        commands.CopyIntoLocalBuffer(data, size, offset, buffer->buffer);
    }
    else
    {
        bblog::error("Failed to allocate memory for batch buffer allocation");
    }

    return { alloc, offset };
}

void BatchAllocator::Free(BatchAllocation alloc)
{
    vmaVirtualFree(_subAllocator, alloc.allocation);
}
