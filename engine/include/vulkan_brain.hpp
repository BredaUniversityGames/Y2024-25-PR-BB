#pragma once
#include "class_decorations.hpp"
#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.h"
#include "engine_init_info.hpp"
#include <optional>
#include "gpu_resources.hpp"
#include "image_resource_manager.hpp"

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool IsComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }

    static QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface);
};

constexpr uint32_t MAX_BINDLESS_RESOURCES = 512;
constexpr uint32_t BINDLESS_TEXTURES_BINDING = 10;

class VulkanBrain
{
public:
    explicit VulkanBrain(const InitInfo& initInfo);
    ~VulkanBrain();
    NON_COPYABLE(VulkanBrain);
    NON_MOVABLE(VulkanBrain);

    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;
    vk::SurfaceKHR surface;
    vk::DescriptorPool descriptorPool;
    vk::CommandPool commandPool;
    vk::DispatchLoaderDynamic dldi;
    VmaAllocator vmaAllocator;
    QueueFamilyIndices queueFamilyIndices;
    uint32_t minUniformBufferOffsetAlignment;

    vk::DescriptorPool bindlessPool;
    vk::DescriptorSetLayout bindlessLayout;
    vk::DescriptorSet bindlessSet;

    ImageResourceManager& ImageResourceManager() const { return _imageResourceManager; }

    void UpdateBindlessSet() const;

private:
    vk::DebugUtilsMessengerEXT _debugMessenger;
    vk::UniqueSampler _sampler;

    ResourceHandle<Image> _fallbackImage;

    const std::vector<const char*> _validationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    const bool _enableValidationLayers =
#if defined(NDEBUG)
        false;
#else
        true;
#endif
    const std::vector<const char*> _deviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if defined(LINUX)
        VK_KHR_MULTIVIEW_EXTENSION_NAME,
        VK_KHR_MAINTENANCE2_EXTENSION_NAME,
#endif
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
        VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        "VK_EXT_descriptor_indexing"
    };

    mutable class ImageResourceManager _imageResourceManager;

    void CreateInstance(const InitInfo& initInfo);
    void PickPhysicalDevice();
    uint32_t RateDeviceSuitability(const vk::PhysicalDevice &device);
    bool ExtensionsSupported(const vk::PhysicalDevice& device);
    bool CheckValidationLayerSupport();
    std::vector<const char*> GetRequiredExtensions(const InitInfo& initInfo);
    void SetupDebugMessenger();
    void CreateDevice();
    void CreateCommandPool();
    void CreateDescriptorPool();
    void CreateBindlessDescriptorSet();
};