#pragma once

#include "application_module.hpp"
#include "common.hpp"

#include "application_module.hpp"
#include "gpu_resources.hpp"
#include "lib/includes_vulkan.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/material_resource_manager.hpp"

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

constexpr uint32_t MAX_BINDLESS_RESOURCES = 128;
enum class BindlessBinding
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
    explicit VulkanContext(const ApplicationModule::VulkanInitInfo& initInfo);

    ~VulkanContext();
    NON_COPYABLE(VulkanContext);
    NON_MOVABLE(VulkanContext);

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

    BufferResourceManager& GetBufferResourceManager() const
    {
        return _bufferResourceManager;
    }

    ImageResourceManager& GetImageResourceManager() const
    {
        return _imageResourceManager;
    }

    MaterialResourceManager& GetMaterialResourceManager() const
    {
        return _materialResourceManager;
    }

    ResourceManager<Mesh>& GetMeshResourceManager() const
    {
        return _meshResourceManager;
    }

    struct DrawStats
    {
        uint32_t indexCount;
        uint32_t drawCalls;
        uint32_t indirectDrawCommands;
        uint32_t debugLines;
    } mutable drawStats;

    void UpdateBindlessSet() const;

private:
    vk::DebugUtilsMessengerEXT _debugMessenger;
    vk::UniqueSampler _sampler;

    ResourceHandle<Image> _fallbackImage;

    mutable std::array<vk::DescriptorImageInfo, MAX_BINDLESS_RESOURCES> _bindlessImageInfos;
    mutable std::array<vk::WriteDescriptorSet, MAX_BINDLESS_RESOURCES> _bindlessImageWrites;

    ResourceHandle<Buffer> _bindlessMaterialBuffer;
    mutable vk::DescriptorBufferInfo _bindlessMaterialInfo;
    mutable vk::WriteDescriptorSet _bindlessMaterialWrite;

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

    mutable BufferResourceManager _bufferResourceManager;
    mutable ImageResourceManager _imageResourceManager;
    mutable MaterialResourceManager _materialResourceManager;
    mutable ResourceManager<Mesh> _meshResourceManager;

    void UpdateBindlessImages() const;
    void UpdateBindlessMaterials() const;

    void CreateInstance(const ApplicationModule::VulkanInitInfo& initInfo);

    void PickPhysicalDevice();

    uint32_t RateDeviceSuitability(const vk::PhysicalDevice& device);

    bool ExtensionsSupported(const vk::PhysicalDevice& device);

    bool CheckValidationLayerSupport();

    std::vector<const char*> GetRequiredExtensions(const ApplicationModule::VulkanInitInfo& initInfo);

    void SetupDebugMessenger();

    void CreateDevice();

    void CreateCommandPool();

    void CreateDescriptorPool();

    void CreateBindlessDescriptorSet();

    void CreateBindlessMaterialBuffer();
};