#pragma once

#include <memory>
#include <optional>
#include <vk_mem_alloc.h>

#include "common.hpp"
#include "gpu_resources.hpp"
#include "vulkan_include.hpp"

struct VulkanInitInfo;

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

constexpr bool ENABLE_VALIDATION_LAYERS =
#if not defined(NDEBUG)
    true;
#else
    false;
#endif

enum class BindlessBinding : std::uint8_t
{
    eColor = 0,
    eDepth,
    eCubemap,
    eShadowmap,
    eMaterial,
    eNone,
};

class VulkanContext
{
public:
    explicit VulkanContext(const VulkanInitInfo& initInfo);

    ~VulkanContext();
    NON_COPYABLE(VulkanContext);
    NON_MOVABLE(VulkanContext);

    vk::Instance Instance() const { return _instance; }
    vk::PhysicalDevice PhysicalDevice() const { return _physicalDevice; }
    vk::Device Device() const { return _device; }
    vk::Queue GraphicsQueue() const { return _graphicsQueue; }
    vk::Queue PresentQueue() const { return _presentQueue; }
    vk::SurfaceKHR Surface() const { return _surface; }
    vk::DescriptorPool DescriptorPool() const { return _descriptorPool; }
    vk::CommandPool CommandPool() const { return _commandPool; }
    vk::DispatchLoaderDynamic Dldi() const { return _dldi; }
    VmaAllocator MemoryAllocator() const { return _vmaAllocator; }
    const QueueFamilyIndices& QueueFamilies() const { return _queueFamilyIndices; }

private:
    friend class Renderer;

    vk::Instance _instance;
    vk::PhysicalDevice _physicalDevice;
    vk::Device _device;
    vk::Queue _graphicsQueue;
    vk::Queue _presentQueue;
    vk::SurfaceKHR _surface;
    vk::DescriptorPool _descriptorPool;
    vk::CommandPool _commandPool;
    vk::DispatchLoaderDynamic _dldi;
    VmaAllocator _vmaAllocator;
    QueueFamilyIndices _queueFamilyIndices;
    uint32_t _minUniformBufferOffsetAlignment;

    bool _validationEnabled = false;
    vk::DebugUtilsMessengerEXT _debugMessenger;

    const std::vector<const char*> _validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> _deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if defined(LINUX)
        VK_KHR_MULTIVIEW_EXTENSION_NAME,
        VK_KHR_MAINTENANCE2_EXTENSION_NAME,
#endif
        VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
        VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
        VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME
    };

    void PickPhysicalDevice();
    uint32_t RateDeviceSuitability(const vk::PhysicalDevice& device);

    bool ExtensionsSupported(const vk::PhysicalDevice& device);
    bool CheckValidationLayerSupport();
    std::vector<const char*> GetRequiredExtensions(const VulkanInitInfo& initInfo);
    void SetupDebugMessenger();

    void CreateInstance(const VulkanInitInfo& initInfo);
    void CreateDevice();
    void CreateCommandPool();
    void CreateDescriptorPool();
};