#pragma once

#include "include.hpp"
#include <vulkan/vulkan.hpp>
#include <magic_enum.hpp>
#include <string>
#include <glm/glm.hpp>
#include <sstream>
#include "spdlog/spdlog.h"
#include "vk_mem_alloc.h"
#include "vulkan_brain.hpp"
#include "mesh.hpp"


namespace util
{
    void VK_ASSERT(vk::Result result, std::string_view message);
    void VK_ASSERT(VkResult result, std::string_view message);
    bool HasStencilComponent(vk::Format format);
    std::optional<vk::Format> FindSupportedFormat(const vk::PhysicalDevice physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
                                                          vk::FormatFeatureFlags features);
    vk::ImageView CreateImageView(vk::Device device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t layer = 0, uint32_t mipCount = 1);
    uint32_t FindMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    void CreateImage(VmaAllocator allocator, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::Image& image, VmaAllocation& allocation, std::string_view name, bool generateMips, VmaMemoryUsage memoryUsage, uint32_t numLayers = 1);
    void CreateBuffer(const VulkanBrain& brain, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::Buffer& buffer, bool mappable, VmaAllocation& allocation, VmaMemoryUsage memoryUsage, std::string_view name);
    vk::CommandBuffer BeginSingleTimeCommands(const VulkanBrain& brain);
    void EndSingleTimeCommands(const VulkanBrain& brain, vk::CommandBuffer commandBuffer);
    void CopyBuffer(vk::CommandBuffer commandBuffer, vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
    MaterialHandle CreateMaterial(const VulkanBrain& brain, const std::array<std::shared_ptr<TextureHandle>, 5>& textures, const MaterialHandle::MaterialInfo& info, vk::Sampler sampler, vk::DescriptorSetLayout materialLayout, std::shared_ptr<MaterialHandle> defaultMaterial = nullptr);
    vk::UniqueSampler CreateSampler(const VulkanBrain& brain, vk::Filter min, vk::Filter mag, vk::SamplerAddressMode addressingMode, vk::SamplerMipmapMode mipmapMode, uint32_t mipLevels);
    void TransitionImageLayout(vk::CommandBuffer commandBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t numLayers = 1, uint32_t mipLevel = 0, uint32_t mipCount = 1,vk::ImageAspectFlagBits imageAspect = vk::ImageAspectFlagBits::eColor);
    void CopyBufferToImage(vk::CommandBuffer commandBuffer, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
    void BeginLabel(vk::Queue queue, std::string_view label, glm::vec3 color, const vk::DispatchLoaderDynamic dldi);
    void EndLabel(vk::Queue queue, const vk::DispatchLoaderDynamic dldi);
    void BeginLabel(vk::CommandBuffer commandBuffer, std::string_view label, glm::vec3 color, const vk::DispatchLoaderDynamic dldi);
    void EndLabel(vk::CommandBuffer commandBuffer, const vk::DispatchLoaderDynamic dldi);
    template <typename T>
    static void NameObject(T object, std::string_view label, vk::Device device, const vk::DispatchLoaderDynamic dldi)
    {
#if defined(NDEBUG)
        return;
#endif
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};

        nameInfo.pObjectName = label.data();
        nameInfo.objectType = object.objectType;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(static_cast<typename T::CType>(object));

        vk::Result result = device.setDebugUtilsObjectNameEXT(&nameInfo, dldi);
        if (result != vk::Result::eSuccess)
            spdlog::warn("Failed debug naming object!");
    }
}
