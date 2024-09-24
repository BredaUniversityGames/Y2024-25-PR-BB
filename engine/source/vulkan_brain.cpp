#include "vulkan_brain.hpp"
#include "vulkan_helper.hpp"
#include "swap_chain.hpp"
#include "vulkan_validation.hpp"
#include <map>


VulkanBrain::VulkanBrain(const InitInfo& initInfo)
    : _imageResourceManager(*this)

{
    CreateInstance(initInfo);
    dldi = vk::DispatchLoaderDynamic { instance, vkGetInstanceProcAddr, device, vkGetDeviceProcAddr };
    SetupDebugMessenger();
    surface = initInfo.retrieveSurface(instance);
    PickPhysicalDevice();
    CreateDevice();

    CreateCommandPool();
    CreateDescriptorPool();

    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo vmaAllocatorCreateInfo {};
    vmaAllocatorCreateInfo.physicalDevice = physicalDevice;
    vmaAllocatorCreateInfo.device = device;
    vmaAllocatorCreateInfo.instance = instance;
    vmaAllocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    vmaAllocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
    vmaCreateAllocator(&vmaAllocatorCreateInfo, &vmaAllocator);

    vk::PhysicalDeviceProperties properties;
    physicalDevice.getProperties(&properties);
    minUniformBufferOffsetAlignment = properties.limits.minUniformBufferOffsetAlignment;

    _sampler = util::CreateSampler(*this, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat,

        vk::SamplerMipmapMode::eLinear, static_cast<uint32_t>(floor(log2(2048))));


    CreateBindlessDescriptorSet();

    std::vector<std::byte> data(2 * 2 * 4 * sizeof(std::byte));

    ImageCreation creation {};

    creation.SetSize(2, 2).SetFlags(vk::ImageUsageFlagBits::eSampled).SetFormat(vk::Format::eR8G8B8A8Unorm).SetData(data.data()).SetName("Fallback texture");

    _fallbackImage = _imageResourceManager.Create(creation);
}

VulkanBrain::~VulkanBrain()
{
    if (ENABLE_VALIDATION_LAYERS)
        instance.destroyDebugUtilsMessengerEXT(_debugMessenger, nullptr, dldi);

    _imageResourceManager.Destroy(_fallbackImage);

    device.destroy(descriptorPool);
    device.destroy(commandPool);

    device.destroy(bindlessLayout);
    device.destroy(bindlessPool);

    _sampler.reset();

    vmaDestroyAllocator(vmaAllocator);

    instance.destroy(surface);
    device.destroy();
    instance.destroy();
}


void VulkanBrain::UpdateBindlessSet() const
{
    for (uint32_t i = 0; i < MAX_BINDLESS_RESOURCES; ++i)
    {
        const Image* image = i < _imageResourceManager.Resources().size()
            ? &_imageResourceManager.Resources()[i].resource.value()
            : _imageResourceManager.Access(_fallbackImage);

        // If it can't be sampled, use the fallback.
        if (!(image->flags & vk::ImageUsageFlagBits::eSampled))
            image = _imageResourceManager.Access(_fallbackImage);

        BindlessBinding dstBinding;
        if (util::GetImageAspectFlags(image->format) == vk::ImageAspectFlagBits::eColor)
            dstBinding = BindlessBinding::eColor;

        if (util::GetImageAspectFlags(image->format) == vk::ImageAspectFlagBits::eDepth)
            dstBinding = BindlessBinding::eDepth;

        if (image->type == ImageType::eCubeMap)
            dstBinding = BindlessBinding::eCubemap;

        _bindlessImageInfos[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        _bindlessImageInfos[i].imageView = image->view;
        _bindlessImageInfos[i].sampler = image->sampler ? image->sampler : *_sampler;

        _bindlessWrites[i].dstSet = bindlessSet;
        _bindlessWrites[i].dstBinding = static_cast<uint32_t>(dstBinding);
        _bindlessWrites[i].dstArrayElement = i;
        _bindlessWrites[i].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        _bindlessWrites[i].descriptorCount = 1;
        _bindlessWrites[i].pImageInfo = &_bindlessImageInfos[i];
    }

    device.updateDescriptorSets(MAX_BINDLESS_RESOURCES, _bindlessWrites.data(), 0, nullptr);
}

void VulkanBrain::CreateInstance(const InitInfo& initInfo)
{
    CheckValidationLayerSupport();
    if (ENABLE_VALIDATION_LAYERS && !CheckValidationLayerSupport())
        throw std::runtime_error("Validation layers requested, but not supported!");

    vk::ApplicationInfo appInfo {};
    appInfo.pApplicationName = "";
    appInfo.applicationVersion = vk::makeApiVersion(0, 0, 0, 0);
    appInfo.engineVersion = vk::makeApiVersion(0, 1, 0, 0);
    appInfo.apiVersion = vk::makeApiVersion(0, 1, 1, 0);
    appInfo.pEngineName = "No engine";

    auto extensions = GetRequiredExtensions(initInfo);
    vk::InstanceCreateInfo createInfo {
        vk::InstanceCreateFlags {},
        &appInfo,
        0, nullptr, // Validation layers.
        static_cast<uint32_t>(extensions.size()), extensions.data() // Extensions.
    };

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
    if (ENABLE_VALIDATION_LAYERS)
    {
        createInfo.enabledLayerCount = _validationLayers.size();
        createInfo.ppEnabledLayerNames = _validationLayers.data();

        util::PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    util::VK_ASSERT(vk::createInstance(&createInfo, nullptr, &instance), "Failed to create vk instance!");
}

void VulkanBrain::PickPhysicalDevice()
{
    std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
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

    physicalDevice = candidates.rbegin()->second;
}

uint32_t VulkanBrain::RateDeviceSuitability(const vk::PhysicalDevice& deviceToRate)
{
    vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures;
    vk::PhysicalDeviceFeatures2 deviceFeatures;
    deviceFeatures.pNext = &indexingFeatures;
    vk::PhysicalDeviceProperties deviceProperties;
    deviceToRate.getProperties(&deviceProperties);
    deviceToRate.getFeatures2(&deviceFeatures);

    QueueFamilyIndices familyIndices = QueueFamilyIndices::FindQueueFamilies(deviceToRate, surface);

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

    // Check for bindless rendering support.
    if(!indexingFeatures.descriptorBindingPartiallyBound || !indexingFeatures.runtimeDescriptorArray)
        return 0;

    // Check support for swap chain.
    SwapChain::SupportDetails swapChainSupportDetails = SwapChain::QuerySupport(deviceToRate, surface);
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

bool VulkanBrain::ExtensionsSupported(const vk::PhysicalDevice& deviceToCheckSupport)
{
    std::vector<vk::ExtensionProperties> availableExtensions = deviceToCheckSupport.enumerateDeviceExtensionProperties();
    std::set<std::string> requiredExtensions { _deviceExtensions.begin(), _deviceExtensions.end() };
    for (const auto& extension : availableExtensions)
        requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

bool VulkanBrain::CheckValidationLayerSupport()
{
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
    bool result = std::all_of(_validationLayers.begin(), _validationLayers.end(), [&availableLayers](const auto& layerName)
        {
        const auto it = std::find_if(availableLayers.begin(), availableLayers.end(), [&layerName](const auto &layer)
        { return strcmp(layerName, layer.layerName) == 0; });

        return it != availableLayers.end(); });

    return result;
}

std::vector<const char*> VulkanBrain::GetRequiredExtensions(const InitInfo& initInfo)
{
    std::vector<const char*> extensions(initInfo.extensions, initInfo.extensions + initInfo.extensionCount);
    if (ENABLE_VALIDATION_LAYERS)
        extensions.emplace_back(vk::EXTDebugUtilsExtensionName);

#ifdef LINUX
    extensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

    return extensions;
}

void VulkanBrain::SetupDebugMessenger()
{
    if (!ENABLE_VALIDATION_LAYERS)
        return;

    vk::DebugUtilsMessengerCreateInfoEXT createInfo {};
    util::PopulateDebugMessengerCreateInfo(createInfo);
    createInfo.pUserData = nullptr;

    util::VK_ASSERT(instance.createDebugUtilsMessengerEXT(&createInfo, nullptr, &_debugMessenger, dldi),
        "Failed to create debug messenger!");
}

void VulkanBrain::CreateDevice()
{
    queueFamilyIndices = QueueFamilyIndices::FindQueueFamilies(physicalDevice, surface);


    vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures;
    vk::PhysicalDeviceFeatures2 deviceFeatures;
    deviceFeatures.pNext = &indexingFeatures;
    physicalDevice.getFeatures2(&deviceFeatures);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos {};
    std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };
    float queuePriority { 1.0f };

    for (uint32_t familyQueueIndex : uniqueQueueFamilies)
        queueCreateInfos.emplace_back(vk::DeviceQueueCreateFlags {}, familyQueueIndex, 1, &queuePriority);

    vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeaturesKhr {};
    dynamicRenderingFeaturesKhr.dynamicRendering = true;

    indexingFeatures.runtimeDescriptorArray = true;
    indexingFeatures.descriptorBindingPartiallyBound = true;

    deviceFeatures.pNext = &dynamicRenderingFeaturesKhr;
    dynamicRenderingFeaturesKhr.pNext = &indexingFeatures;


    vk::DeviceCreateInfo createInfo {};

    createInfo.pNext = &deviceFeatures;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = nullptr; // Shouldn't be set because we already pass a pnext for DeviceFeatures2.
    createInfo.enabledExtensionCount = static_cast<uint32_t>(_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = _deviceExtensions.data();

    spdlog::info("Validation layers enabled: {}", ENABLE_VALIDATION_LAYERS ? "TRUE" : "FALSE");

    if (ENABLE_VALIDATION_LAYERS)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());
        createInfo.ppEnabledLayerNames = _validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    util::VK_ASSERT(physicalDevice.createDevice(&createInfo, nullptr, &device), "Failed creating a logical device!");

    device.getQueue(queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
    device.getQueue(queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
}

void VulkanBrain::CreateCommandPool()
{
    vk::CommandPoolCreateInfo commandPoolCreateInfo {};
    commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    util::VK_ASSERT(device.createCommandPool(&commandPoolCreateInfo, nullptr, &commandPool), "Failed creating command pool!");
}

void VulkanBrain::CreateDescriptorPool()
{
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        { vk::DescriptorType::eSampler, 1000 },
        { vk::DescriptorType::eCombinedImageSampler, 1000 },
        { vk::DescriptorType::eSampledImage, 1000 },
        { vk::DescriptorType::eStorageImage, 1000 },
        { vk::DescriptorType::eUniformTexelBuffer, 1000 },
        { vk::DescriptorType::eStorageTexelBuffer, 1000 },
        { vk::DescriptorType::eUniformBuffer, 1000 },
        { vk::DescriptorType::eStorageBuffer, 1000 },
        { vk::DescriptorType::eUniformBufferDynamic, 1000 },
        { vk::DescriptorType::eStorageBufferDynamic, 1000 },
        { vk::DescriptorType::eInputAttachment, 1000 }
    };

    vk::DescriptorPoolCreateInfo createInfo {};
    createInfo.poolSizeCount = poolSizes.size();
    createInfo.pPoolSizes = poolSizes.data();
    createInfo.maxSets = 200;
    createInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    util::VK_ASSERT(device.createDescriptorPool(&createInfo, nullptr, &descriptorPool), "Failed creating descriptor pool!");
}

void VulkanBrain::CreateBindlessDescriptorSet()
{

    std::array<vk::DescriptorPoolSize, 3> poolSizes = {
        vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_RESOURCES },
        vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_RESOURCES },
        vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_RESOURCES },
    };

    vk::DescriptorPoolCreateInfo poolCreateInfo {};
    poolCreateInfo.flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
    poolCreateInfo.maxSets = MAX_BINDLESS_RESOURCES * poolSizes.size();
    poolCreateInfo.poolSizeCount = poolSizes.size();
    poolCreateInfo.pPoolSizes = poolSizes.data();
    util::VK_ASSERT(device.createDescriptorPool(&poolCreateInfo, nullptr, &bindlessPool), "Failed creating bindless pool!");

    std::array<vk::DescriptorSetLayoutBinding, 3> bindings;
    vk::DescriptorSetLayoutBinding& combinedImageSampler = bindings[0];
    combinedImageSampler.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    combinedImageSampler.descriptorCount = MAX_BINDLESS_RESOURCES;
    combinedImageSampler.binding = static_cast<uint32_t>(BindlessBinding::eColor);
    combinedImageSampler.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;

    vk::DescriptorSetLayoutBinding& depthImageBinding = bindings[1];
    depthImageBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    depthImageBinding.descriptorCount = MAX_BINDLESS_RESOURCES;
    depthImageBinding.binding = static_cast<uint32_t>(BindlessBinding::eDepth);
    depthImageBinding.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;

    vk::DescriptorSetLayoutBinding& cubemapBinding = bindings[2];
    cubemapBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    cubemapBinding.descriptorCount = MAX_BINDLESS_RESOURCES;
    cubemapBinding.binding = static_cast<uint32_t>(BindlessBinding::eCubemap);
    cubemapBinding.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;

    vk::DescriptorSetLayoutCreateInfo layoutCreateInfo {};
    layoutCreateInfo.bindingCount = bindings.size();
    layoutCreateInfo.pBindings = bindings.data();
    layoutCreateInfo.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;

    std::array<vk::DescriptorBindingFlagsEXT, bindings.size()> bindingFlags = {
        vk::DescriptorBindingFlagBits::ePartiallyBound,
        vk::DescriptorBindingFlagBits::ePartiallyBound,
        vk::DescriptorBindingFlagBits::ePartiallyBound
    };

    vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT extInfo {};
    extInfo.bindingCount = bindings.size();
    extInfo.pBindingFlags = bindingFlags.data();

    layoutCreateInfo.pNext = &extInfo;

    util::VK_ASSERT(device.createDescriptorSetLayout(&layoutCreateInfo, nullptr, &bindlessLayout),
        "Failed creating bindless descriptor set layout.");

    vk::DescriptorSetAllocateInfo allocInfo {};

    allocInfo.descriptorPool = bindlessPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &bindlessLayout;

    util::VK_ASSERT(device.allocateDescriptorSets(&allocInfo, &bindlessSet), "Failed creating bindless descriptor set!");
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
