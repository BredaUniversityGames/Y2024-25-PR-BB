#pragma once

#include "log.hpp"
#include "mesh.hpp"
#include "spirv_reflect.h"
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
bool HasStencilComponent(vk::Format format);
std::optional<vk::Format> FindSupportedFormat(const vk::PhysicalDevice physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
    vk::FormatFeatureFlags features);
uint32_t FindMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
void CreateBuffer(const VulkanContext& brain, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::Buffer& buffer, bool mappable, VmaAllocation& allocation, VmaMemoryUsage memoryUsage, std::string_view name);
vk::CommandBuffer BeginSingleTimeCommands(const VulkanContext& brain);
void EndSingleTimeCommands(const VulkanContext& brain, vk::CommandBuffer commandBuffer);
void CopyBuffer(vk::CommandBuffer commandBuffer, vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, uint32_t offset = 0);
vk::UniqueSampler CreateSampler(const VulkanContext& brain, vk::Filter min, vk::Filter mag, vk::SamplerAddressMode addressingMode, vk::SamplerMipmapMode mipmapMode, uint32_t mipLevels);
ImageLayoutTransitionState GetImageLayoutTransitionSourceState(vk::ImageLayout sourceLayout);
ImageLayoutTransitionState GetImageLayoutTransitionDestinationState(vk::ImageLayout destinationLayout);
void InitializeImageMemoryBarrier(vk::ImageMemoryBarrier2& barrier, vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t numLayers = 1, uint32_t mipLevel = 0, uint32_t mipCount = 1, vk::ImageAspectFlagBits imageAspect = vk::ImageAspectFlagBits::eColor);
void TransitionImageLayout(vk::CommandBuffer commandBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t numLayers = 1, uint32_t mipLevel = 0, uint32_t mipCount = 1, vk::ImageAspectFlagBits imageAspect = vk::ImageAspectFlagBits::eColor);
void CopyBufferToImage(vk::CommandBuffer commandBuffer, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
void BeginLabel(vk::Queue queue, std::string_view label, glm::vec3 color, const vk::DispatchLoaderDynamic dldi);
void EndLabel(vk::Queue queue, const vk::DispatchLoaderDynamic dldi);
void BeginLabel(vk::CommandBuffer commandBuffer, std::string_view label, glm::vec3 color, const vk::DispatchLoaderDynamic dldi);
void EndLabel(vk::CommandBuffer commandBuffer, const vk::DispatchLoaderDynamic dldi);
vk::ImageAspectFlags GetImageAspectFlags(vk::Format format);
template <typename T>
static void NameObject(T object, std::string_view label, const VulkanContext& brain)
{
#if defined(NDEBUG)
    return;
#endif
    vk::DebugUtilsObjectNameInfoEXT nameInfo {};

    nameInfo.pObjectName = label.data();
    nameInfo.objectType = object.objectType;
    nameInfo.objectHandle = reinterpret_cast<uint64_t>(static_cast<typename T::CType>(object));

    vk::Result result = brain.device.setDebugUtilsObjectNameEXT(&nameInfo, brain.dldi);
    if (result != vk::Result::eSuccess)
        spdlog::warn("Failed debug naming object!");
}

uint32_t FormatSize(vk::Format format);
}
