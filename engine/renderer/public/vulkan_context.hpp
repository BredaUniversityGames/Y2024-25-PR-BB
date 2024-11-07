#pragma once

#include <memory>
#include <optional>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "application_module.hpp"
#include "common.hpp"
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

enum class BindlessBinding : std::uint8_t
{
    eColor = 0,
    eDepth,
    eCubemap,
    eShadowmap,
    eMaterial,
    eNone,
};

class VulkanContext : public std::enable_shared_from_this<VulkanContext> // TODO: Remove `shared_from_this` since technically not required.
{
public:
    explicit VulkanContext(const ApplicationModule::VulkanInitInfo& initInfo);

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
    vk::DescriptorSetLayout BindlessLayout() const { return _bindlessLayout; }
    vk::DescriptorSet BindlessSet() const { return _bindlessSet; }

    BufferResourceManager& GetBufferResourceManager() { return *_bufferResourceManager; }
    ImageResourceManager& GetImageResourceManager() { return *_imageResourceManager; }
    MaterialResourceManager& GetMaterialResourceManager() { return *_materialResourceManager; }
    ResourceManager<Mesh>& GetMeshResourceManager() { return *_meshResourceManager; }

    void UpdateBindlessSet();

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

    vk::DescriptorPool _bindlessPool;
    vk::DescriptorSetLayout _bindlessLayout;
    vk::DescriptorSet _bindlessSet;

    vk::DebugUtilsMessengerEXT _debugMessenger;
    vk::UniqueSampler _sampler;

    ResourceHandle<Image> _fallbackImage;

    std::array<vk::DescriptorImageInfo, MAX_BINDLESS_RESOURCES> _bindlessImageInfos;
    std::array<vk::WriteDescriptorSet, MAX_BINDLESS_RESOURCES> _bindlessImageWrites;

    ResourceHandle<Buffer> _bindlessMaterialBuffer;
    vk::DescriptorBufferInfo _bindlessMaterialInfo;
    vk::WriteDescriptorSet _bindlessMaterialWrite;

    struct DrawStats
    {
        uint32_t indexCount;
        uint32_t drawCalls;
        uint32_t indirectDrawCommands;
        uint32_t debugLines;
    } _drawStats;

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

    std::unique_ptr<BufferResourceManager> _bufferResourceManager;
    std::unique_ptr<ImageResourceManager> _imageResourceManager;
    std::unique_ptr<MaterialResourceManager> _materialResourceManager;
    std::unique_ptr<ResourceManager<Mesh>> _meshResourceManager;

    void Init();

    void UpdateBindlessImages();
    void UpdateBindlessMaterials();

    void PickPhysicalDevice();
    uint32_t RateDeviceSuitability(const vk::PhysicalDevice& device);

    bool ExtensionsSupported(const vk::PhysicalDevice& device);
    bool CheckValidationLayerSupport();
    std::vector<const char*> GetRequiredExtensions(const ApplicationModule::VulkanInitInfo& initInfo);
    void SetupDebugMessenger();

    void CreateInstance(const ApplicationModule::VulkanInitInfo& initInfo);
    void CreateDevice();
    void CreateCommandPool();
    void CreateDescriptorPool();
    void CreateBindlessDescriptorSet();
    void CreateBindlessMaterialBuffer();
};