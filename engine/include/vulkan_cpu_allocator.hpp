#pragma once
#include <cstddef>

namespace detail
{
void* VKAPI_CALL vk_alloc(void* user, size_t size, size_t alignment, VkSystemAllocationScope scope);
void* VKAPI_CALL vk_realloc(void* user, void* source, size_t size, size_t alignment, VkSystemAllocationScope scope);
void VKAPI_CALL vk_free(void* user, void* ptr);
void VKAPI_CALL vk_internal_alloc_notify(void* user, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope);
void VKAPI_CALL vk_internal_free_notify(void* user, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope);
}

inline vk::AllocationCallbacks MakeVkAllocationCallbacks()
{
    return vk::AllocationCallbacks {
        .pUserData = nullptr, // NULL, we don't have any user data
        .pfnAllocation = detail::vk_alloc,
        .pfnReallocation = detail::vk_realloc,
        .pfnFree = detail::vk_free,
        .pfnInternalAllocation = nullptr, // NULL, we don't want any notifications on this callback
        .pfnInternalFree = nullptr, // NULL, we don't want any notifications on this callback
    };
}
//
// namespace detail
// {
// void* VKAPI_CALL all(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
// {
//     // Align the memory if necessary
//     printf("Allocating %zu bytes\n", size);
//     return aligned_alloc(alignment, size);
// }
// // Custom reallocation function
// void* VKAPI_CALL myRealloc(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
// {
//     printf("Reallocating to %zu bytes\n", size);
//     if (pOriginal == NULL)
//     {
//         return aligned_alloc(alignment, size); // Initial allocation case
//     }
//     return realloc(pOriginal, size);
// }
//
// // Custom free function
// void VKAPI_CALL myFree(void* pUserData, void* pMemory)
// {
//     printf("Freeing memory\n");
//     free(pMemory);
// }
//
// // Internal allocation notification (optional)
// void VKAPI_CALL myInternalAllocNotify(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
// {
//     printf("Internal allocation of %zu bytes\n", size);
// }
//
// // Internal free notification (optional)
// void VKAPI_CALL myInternalFreeNotify(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
// {
//     printf("Internal freeing of %zu bytes\n", size);
// }
