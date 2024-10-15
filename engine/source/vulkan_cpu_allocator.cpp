#include "vulkan_cpu_allocator.hpp"
#include "profile_macros.hpp"
#include <cstdlib>

void* VKAPI_CALL detail::vk_alloc(void* user, size_t size, size_t alignment, VkSystemAllocationScope scope)
{
    void* mem = std::malloc(size);
    TracyAlloc(mem, size);

    return mem;
}

void* VKAPI_CALL detail::vk_realloc(void* user, void* source, size_t size, size_t alignment, VkSystemAllocationScope scope)
{
    TracyFree(source);
    void* newMem = std::realloc(source, size);
    TracyAlloc(newMem, size);

    return newMem;
}

void VKAPI_CALL detail::vk_free(void* user, void* ptr)
{
    TracyFree(ptr);
    std::free(ptr);
}

void VKAPI_CALL detail::vk_internal_alloc_notify(void* user, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope)
{
}

void VKAPI_CALL detail::vk_internal_free_notify(void* user, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope)
{
}