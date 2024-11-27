#include "vulkan_context.hpp"

#include <map>
#include <set>
#include <vulkan/vulkan.hpp>

#include "application_module.hpp"
#include "log.hpp"
#include "pipeline_builder.hpp"
#include "swap_chain.hpp"
#include "vulkan_helper.hpp"
#include "vulkan_validation.hpp"

VulkanContext::VulkanContext(const VulkanInitInfo& initInfo)
{
    CreateInstance(initInfo);
    _dldi = vk::DispatchLoaderDynamic { _instance, vkGetInstanceProcAddr, _device, vkGetDeviceProcAddr };
    SetupDebugMessenger();
    _surface = initInfo.retrieveSurface(_instance);

    PickPhysicalDevice();
    CreateDevice();

    CreateCommandPool();
    CreateDescriptorPool();

    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo vmaAllocatorCreateInfo {};
    vmaAllocatorCreateInfo.physicalDevice = _physicalDevice;
    vmaAllocatorCreateInfo.device = _device;
    vmaAllocatorCreateInfo.instance = _instance;
    vmaAllocatorCreateInfo.vulkanApiVersion = vk::makeApiVersion(0, 1, 3, 0);
    vmaAllocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
    vmaCreateAllocator(&vmaAllocatorCreateInfo, &_vmaAllocator);

    vk::PhysicalDeviceProperties properties;
    _physicalDevice.getProperties(&properties);
    _minUniformBufferOffsetAlignment = properties.limits.minUniformBufferOffsetAlignment;
}

VulkanContext::~VulkanContext()
{
    if (ENABLE_VALIDATION_LAYERS)
        _instance.destroyDebugUtilsMessengerEXT(_debugMessenger, nullptr, _dldi);
    ;

    _device.destroy(_descriptorPool);
    _device.destroy(_commandPool);

    vmaDestroyAllocator(_vmaAllocator);

    _instance.destroy(_surface);
    _device.destroy();
    _instance.destroy();
}

void VulkanContext::CreateInstance(const VulkanInitInfo& initInfo)
{
    CheckValidationLayerSupport();
    if (ENABLE_VALIDATION_LAYERS && !CheckValidationLayerSupport())
        throw std::runtime_error("Validation layers requested, but not supported!");

    vk::ApplicationInfo appInfo {
        .pApplicationName = "Sample Application Name",
        .applicationVersion = vk::makeApiVersion(0, 0, 0, 0),
        .pEngineName = "BB Engine",
        .engineVersion = vk::makeApiVersion(0, 1, 0, 0),
        .apiVersion = vk::makeApiVersion(0, 1, 3, 0),
    };

    vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
        structureChain;

    auto extensions = GetRequiredExtensions(initInfo);
    structureChain.assign({
        .flags = vk::InstanceCreateFlags {},
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr, // Validation layers.
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data() // Extensions.
    });

    auto& createInfo = structureChain.get<vk::InstanceCreateInfo>();

    if (ENABLE_VALIDATION_LAYERS)
    {
        createInfo.enabledLayerCount = _validationLayers.size();
        createInfo.ppEnabledLayerNames = _validationLayers.data();

        util::PopulateDebugMessengerCreateInfo(structureChain.get<vk::DebugUtilsMessengerCreateInfoEXT>());
    }
    else
    {
        // Make sure the debug extension is unlinked.
        structureChain.unlink<vk::DebugUtilsMessengerCreateInfoEXT>();
        createInfo.enabledLayerCount = 0;
    }

    util::VK_ASSERT(vk::createInstance(&createInfo, nullptr, &_instance), "Failed to create vk instance!");
}

void VulkanContext::PickPhysicalDevice()
{
    std::vector<vk::PhysicalDevice> devices = _instance.enumeratePhysicalDevices();
    if (devices.empty())
        throw std::runtime_error("No GPU's with Vulkan support available!");

    std::multimap<int, vk::PhysicalDevice> candidates {};

    for (const auto& device : devices)
    {
        uint32_t score = RateDeviceSuitability(device);
        if (score > 0)
            candidates.emplace(score, device);
    }
    if (candidates.empty())
        throw std::runtime_error("Failed finding suitable device!");

    _physicalDevice = candidates.rbegin()->second;
}

uint32_t VulkanContext::RateDeviceSuitability(const vk::PhysicalDevice& deviceToRate)
{
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceDescriptorIndexingFeatures> structureChain;

    auto& indexingFeatures = structureChain.get<vk::PhysicalDeviceDescriptorIndexingFeatures>();
    auto& deviceFeatures = structureChain.get<vk::PhysicalDeviceFeatures2>();

    vk::PhysicalDeviceProperties deviceProperties;
    deviceToRate.getProperties(&deviceProperties);
    deviceToRate.getFeatures2(&deviceFeatures);

    QueueFamilyIndices familyIndices = QueueFamilyIndices::FindQueueFamilies(deviceToRate, _surface);

    uint32_t score { 0 };

    // Failed if geometry shader is not supported.
    if (!deviceFeatures.features.geometryShader)
        return 0;

    // Failed if graphics family queue is not supported.
    if (!familyIndices.IsComplete())
        return 0;

    // Failed if no extensions are supported.
    if (!ExtensionsSupported(deviceToRate))
        return 0;

    // Check for bindless rendering support.
    if (!indexingFeatures.descriptorBindingPartiallyBound || !indexingFeatures.runtimeDescriptorArray)
        return 0;

    // Check support for swap chain.
    SwapChain::SupportDetails swapChainSupportDetails = SwapChain::QuerySupport(deviceToRate, _surface);
    bool swapChainUnsupported = swapChainSupportDetails.formats.empty() || swapChainSupportDetails.presentModes.empty();
    if (swapChainUnsupported)
        return 0;

    // Favor discrete GPUs above all else.
    if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        score += 50000;

    // Slightly favor integrated GPUs.
    if (deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
        score += 30000;

    score += deviceProperties.limits.maxImageDimension2D;

    return score;
}

bool VulkanContext::ExtensionsSupported(const vk::PhysicalDevice& deviceToCheckSupport)
{
    std::vector<vk::ExtensionProperties> availableExtensions = deviceToCheckSupport.enumerateDeviceExtensionProperties();
    std::set<std::string> requiredExtensions { _deviceExtensions.begin(), _deviceExtensions.end() };
    for (const auto& extension : availableExtensions)
        requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

bool VulkanContext::CheckValidationLayerSupport()
{
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
    bool result = std::all_of(_validationLayers.begin(), _validationLayers.end(), [&availableLayers](const auto& layerName)
        {
        const auto it = std::find_if(availableLayers.begin(), availableLayers.end(), [&layerName](const auto &layer)
        { return strcmp(layerName, layer.layerName) == 0; });

        return it != availableLayers.end(); });

    return result;
}

std::vector<const char*> VulkanContext::GetRequiredExtensions(const VulkanInitInfo& initInfo)
{
    std::vector<const char*> extensions(initInfo.extensions, initInfo.extensions + initInfo.extensionCount);
    if (ENABLE_VALIDATION_LAYERS)
        extensions.emplace_back(vk::EXTDebugUtilsExtensionName);

#ifdef LINUX
    extensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

    return extensions;
}

void VulkanContext::SetupDebugMessenger()
{
    if (!ENABLE_VALIDATION_LAYERS)
        return;

    vk::DebugUtilsMessengerCreateInfoEXT createInfo {};
    util::PopulateDebugMessengerCreateInfo(createInfo);
    createInfo.pUserData = nullptr;

    util::VK_ASSERT(_instance.createDebugUtilsMessengerEXT(&createInfo, nullptr, &_debugMessenger, _dldi),
        "Failed to create debug messenger!");
}

void VulkanContext::CreateDevice()
{
    _queueFamilyIndices = QueueFamilyIndices::FindQueueFamilies(_physicalDevice, _surface);
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos {};
    std::set<uint32_t> uniqueQueueFamilies = { _queueFamilyIndices.graphicsFamily.value(), _queueFamilyIndices.presentFamily.value() };
    float queuePriority { 1.0f };

    for (uint32_t familyQueueIndex : uniqueQueueFamilies)
        queueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo { .flags = vk::DeviceQueueCreateFlags {}, .queueFamilyIndex = familyQueueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority });

    vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceDynamicRenderingFeaturesKHR, vk::PhysicalDeviceDescriptorIndexingFeatures, vk::PhysicalDeviceSynchronization2Features> structureChain;

    auto& synchronization2Features = structureChain.get<vk::PhysicalDeviceSynchronization2Features>();
    synchronization2Features.synchronization2 = true;

    auto& indexingFeatures = structureChain.get<vk::PhysicalDeviceDescriptorIndexingFeatures>();
    indexingFeatures.runtimeDescriptorArray = true;
    indexingFeatures.descriptorBindingPartiallyBound = true;

    auto& dynamicRenderingFeaturesKhr = structureChain.get<vk::PhysicalDeviceDynamicRenderingFeaturesKHR>();
    dynamicRenderingFeaturesKhr.dynamicRendering = true;

    auto& deviceFeatures = structureChain.get<vk::PhysicalDeviceFeatures2>();
    _physicalDevice.getFeatures2(&deviceFeatures);

    auto& createInfo = structureChain.get<vk::DeviceCreateInfo>();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = nullptr;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = _deviceExtensions.data();

    bblog::info("Validation layers enabled: {}", ENABLE_VALIDATION_LAYERS ? "TRUE" : "FALSE");

    if (ENABLE_VALIDATION_LAYERS)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());
        createInfo.ppEnabledLayerNames = _validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    util::VK_ASSERT(_physicalDevice.createDevice(&createInfo, nullptr, &_device), "Failed creating a logical device!");

    _device.getQueue(_queueFamilyIndices.graphicsFamily.value(), 0, &_graphicsQueue);
    _device.getQueue(_queueFamilyIndices.presentFamily.value(), 0, &_presentQueue);
}

void VulkanContext::CreateCommandPool()
{
    vk::CommandPoolCreateInfo commandPoolCreateInfo {};
    commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolCreateInfo.queueFamilyIndex = _queueFamilyIndices.graphicsFamily.value();

    util::VK_ASSERT(_device.createCommandPool(&commandPoolCreateInfo, nullptr, &_commandPool), "Failed creating command pool!");
}

void VulkanContext::CreateDescriptorPool()
{
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        { vk::DescriptorType::eSampler, 1024 },
        { vk::DescriptorType::eCombinedImageSampler, 1024 },
        { vk::DescriptorType::eSampledImage, 1024 },
        { vk::DescriptorType::eStorageImage, 1024 },
        { vk::DescriptorType::eUniformTexelBuffer, 1024 },
        { vk::DescriptorType::eStorageTexelBuffer, 1024 },
        { vk::DescriptorType::eUniformBuffer, 1024 },
        { vk::DescriptorType::eStorageBuffer, 1024 },
        { vk::DescriptorType::eUniformBufferDynamic, 1024 },
        { vk::DescriptorType::eStorageBufferDynamic, 1024 },
        { vk::DescriptorType::eInputAttachment, 1024 }
    };

    vk::DescriptorPoolCreateInfo createInfo {};
    createInfo.poolSizeCount = poolSizes.size();
    createInfo.pPoolSizes = poolSizes.data();
    createInfo.maxSets = 256;
    createInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    util::VK_ASSERT(_device.createDescriptorPool(&createInfo, nullptr, &_descriptorPool), "Failed creating descriptor pool!");
}

QueueFamilyIndices QueueFamilyIndices::FindQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    QueueFamilyIndices indices {};

    uint32_t queueFamilyCount { 0 };
    device.getQueueFamilyProperties(&queueFamilyCount, nullptr);

    std::vector<vk::QueueFamilyProperties> queueFamilies(queueFamilyCount);
    device.getQueueFamilyProperties(&queueFamilyCount, queueFamilies.data());

    for (size_t i = 0; i < queueFamilies.size(); ++i)
    {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)
            indices.graphicsFamily = i;

        if (!indices.presentFamily.has_value())
        {
            vk::Bool32 supported;
            util::VK_ASSERT(device.getSurfaceSupportKHR(i, surface, &supported),
                "Failed querying surface support on physical device!");
            if (supported)
                indices.presentFamily = i;
        }

        if (indices.IsComplete())
            break;
    }

    return indices;
}
