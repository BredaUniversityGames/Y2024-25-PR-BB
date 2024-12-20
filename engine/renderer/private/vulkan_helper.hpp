#pragma once

#include "log.hpp"
#include "spirv_reflect.h"
#include "vertex.hpp"
#include "vulkan_context.hpp"
#include <glm/glm.hpp>
#include <magic_enum.hpp>

namespace util
{
struct ImageLayoutTransitionState
{
    vk::PipelineStageFlags2 pipelineStage {};
    vk::AccessFlags2 accessFlags {};
};

void VK_ASSERT(vk::Result result, std::string_view message);
void VK_ASSERT(VkResult result, std::string_view message);
void VK_ASSERT(SpvReflectResult result, std::string_view message);

VkResult vmaCreateBuffer(VmaAllocator allocator,
    const VkBufferCreateInfo* pBufferCreateInfo,
    const VmaAllocationCreateInfo* pAllocationCreateInfo,
    VkBuffer* pBuffer,
    VmaAllocation* pAllocation,
    VmaAllocationInfo* pAllocationInfo);
void vmaDestroyBuffer(
    VmaAllocator allocator,
    VkBuffer buffer,
    VmaAllocation allocation);
VkResult vmaCreateImage(VmaAllocator allocator,
    const VkImageCreateInfo* pImageCreateInfo,
    const VmaAllocationCreateInfo* pAllocationCreateInfo,
    VkImage* pImage,
    VmaAllocation* pAllocation,
    VmaAllocationInfo* pAllocationInfo);
void vmaDestroyImage(
    VmaAllocator allocator,
    VkImage image,
    VmaAllocation allocation);

bool HasStencilComponent(vk::Format format);
std::optional<vk::Format> FindSupportedFormat(const vk::PhysicalDevice physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
    vk::FormatFeatureFlags features);
uint32_t FindMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
void CreateBuffer(std::shared_ptr<VulkanContext> context, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::Buffer& buffer, bool mappable, VmaAllocation& allocation, VmaMemoryUsage memoryUsage, std::string_view name);
vk::CommandBuffer BeginSingleTimeCommands(std::shared_ptr<VulkanContext> context);
void EndSingleTimeCommands(std::shared_ptr<VulkanContext> context, vk::CommandBuffer commandBuffer);
void CopyBuffer(vk::CommandBuffer commandBuffer, vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, uint32_t offset = 0);
ImageLayoutTransitionState GetImageLayoutTransitionSourceState(vk::ImageLayout sourceLayout);
ImageLayoutTransitionState GetImageLayoutTransitionDestinationState(vk::ImageLayout destinationLayout);
void InitializeImageMemoryBarrier(vk::ImageMemoryBarrier2& barrier, vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t numLayers = 1, uint32_t mipLevel = 0, uint32_t mipCount = 1, vk::ImageAspectFlagBits imageAspect = vk::ImageAspectFlagBits::eColor);
void TransitionImageLayout(vk::CommandBuffer commandBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t numLayers = 1, uint32_t mipLevel = 0, uint32_t mipCount = 1, vk::ImageAspectFlagBits imageAspect = vk::ImageAspectFlagBits::eColor);
void CopyBufferToImage(vk::CommandBuffer commandBuffer, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
void CopyImageToImage(vk::CommandBuffer commandBuffer, vk::Image srcImage, vk::Image dstImage, vk::Extent2D srcSize, vk::Extent2D dstSize);
void BeginLabel(vk::Queue queue, std::string_view label, glm::vec3 color, const vk::DispatchLoaderDynamic dldi);
void EndLabel(vk::Queue queue, const vk::DispatchLoaderDynamic dldi);
void BeginLabel(vk::CommandBuffer commandBuffer, std::string_view label, glm::vec3 color, const vk::DispatchLoaderDynamic dldi);
void EndLabel(vk::CommandBuffer commandBuffer, const vk::DispatchLoaderDynamic dldi);
vk::ImageAspectFlags GetImageAspectFlags(vk::Format format);
template <typename T>
static void NameObject(T object, std::string_view label, std::shared_ptr<VulkanContext> context)
{
#if defined(NDEBUG)
    return;
#endif
    vk::DebugUtilsObjectNameInfoEXT nameInfo {};

    nameInfo.pObjectName = label.data();
    nameInfo.objectType = object.objectType;
    nameInfo.objectHandle = reinterpret_cast<uint64_t>(static_cast<typename T::CType>(object));

    vk::Result result = context->Device().setDebugUtilsObjectNameEXT(&nameInfo, context->Dldi());
    if (result != vk::Result::eSuccess)
        spdlog::warn("Failed debug naming object!");
}

uint32_t FormatSize(vk::Format format);
}
