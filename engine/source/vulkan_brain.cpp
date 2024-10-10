#include "vulkan_brain.hpp"
#include "vulkan_helper.hpp"
#include "swap_chain.hpp"
#include "vulkan_validation.hpp"
#include <map>

VulkanBrain::VulkanBrain(const InitInfo& initInfo)
    : _bufferResourceManager(*this)
    , _imageResourceManager(*this)
    , _materialResourceManager(*this)
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

    CreateBindlessMaterialBuffer();
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

    _bufferResourceManager.Destroy(_bindlessMaterialBuffer);

    _sampler.reset();

    vmaDestroyAllocator(vmaAllocator);

    instance.destroy(surface);
    device.destroy();
    instance.destroy();
}

void VulkanBrain::UpdateBindlessSet() const
{
    UpdateBindlessImages();
    UpdateBindlessMaterials();
}

void VulkanBrain::UpdateBindlessImages() const
{
    for (uint32_t i = 0; i < MAX_BINDLESS_RESOURCES; ++i)
    {
        const Image* image = i < _imageResourceManager.Resources().size()
            ? &_imageResourceManager.Resources()[i].resource.value()
            : _imageResourceManager.Access(_fallbackImage);

        // If it can't be sampled, use the fallback.
        if (!(image->flags & vk::ImageUsageFlagBits::eSampled))
            image = _imageResourceManager.Access(_fallbackImage);

        BindlessBinding dstBinding = BindlessBinding::eNone;

        if (util::GetImageAspectFlags(image->format) == vk::ImageAspectFlagBits::eColor)
            dstBinding = BindlessBinding::eColor;

        if (util::GetImageAspectFlags(image->format) == vk::ImageAspectFlagBits::eDepth)
            dstBinding = BindlessBinding::eDepth;

        if (image->type == ImageType::eCubeMap)
            dstBinding = BindlessBinding::eCubemap;

        if (image->type == ImageType::eShadowMap)
            dstBinding = BindlessBinding::eShadowmap;

        _bindlessImageInfos[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        _bindlessImageInfos[i].imageView = image->view;
        _bindlessImageInfos[i].sampler = image->sampler ? image->sampler : *_sampler;

        _bindlessImageWrites[i].dstSet = bindlessSet;
        _bindlessImageWrites[i].dstBinding = static_cast<uint32_t>(dstBinding);
        _bindlessImageWrites[i].dstArrayElement = i;
        _bindlessImageWrites[i].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        _bindlessImageWrites[i].descriptorCount = 1;
        _bindlessImageWrites[i].pImageInfo = &_bindlessImageInfos[i];
    }

    device.updateDescriptorSets(MAX_BINDLESS_RESOURCES, _bindlessImageWrites.data(), 0, nullptr);
}

void VulkanBrain::UpdateBindlessMaterials() const
{
    assert(_materialResourceManager.Resources().size() < MAX_BINDLESS_RESOURCES && "There are more materials used than the amount that can be stored on the GPU.");

    std::array<Material::GPUInfo, MAX_BINDLESS_RESOURCES> materialGPUData;

    for (uint32_t i = 0; i < _materialResourceManager.Resources().size(); ++i)
    {
        const Material* material = &_materialResourceManager.Resources()[i].resource.value();
        materialGPUData[i] = material->gpuInfo;
    }

    const Buffer* buffer = _bufferResourceManager.Access(_bindlessMaterialBuffer);
    std::memcpy(buffer->mappedPtr, materialGPUData.data(), _materialResourceManager.Resources().size() * sizeof(Material::GPUInfo));

    _bindlessMaterialInfo.buffer = buffer->buffer;
    _bindlessMaterialInfo.offset = 0;
    _bindlessMaterialInfo.range = sizeof(Material::GPUInfo) * _materialResourceManager.Resources().size();

    _bindlessMaterialWrite.dstSet = bindlessSet;
    _bindlessMaterialWrite.dstBinding = static_cast<uint32_t>(BindlessBinding::eMaterial);
    _bindlessMaterialWrite.dstArrayElement = 0;
    _bindlessMaterialWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    _bindlessMaterialWrite.descriptorCount = 1;
    _bindlessMaterialWrite.pBufferInfo = &_bindlessMaterialInfo;

    device.updateDescriptorSets(1, &_bindlessMaterialWrite, 0, nullptr);
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

    vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> structureChain;

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
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceDescriptorIndexingFeatures> structureChain;

    auto& indexingFeatures = structureChain.get<vk::PhysicalDeviceDescriptorIndexingFeatures>();
    auto& deviceFeatures = structureChain.get<vk::PhysicalDeviceFeatures2>();

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
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos {};
    std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };
    float queuePriority { 1.0f };

    for (uint32_t familyQueueIndex : uniqueQueueFamilies)
        queueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo { .flags = vk::DeviceQueueCreateFlags {}, .queueFamilyIndex = familyQueueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority });

    vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceDynamicRenderingFeaturesKHR, vk::PhysicalDeviceDescriptorIndexingFeatures> structureChain;
    auto& indexingFeatures = structureChain.get<vk::PhysicalDeviceDescriptorIndexingFeatures>();
    auto& deviceFeatures = structureChain.get<vk::PhysicalDeviceFeatures2>();
    physicalDevice.getFeatures2(&deviceFeatures);

    auto& dynamicRenderingFeaturesKhr = structureChain.get<vk::PhysicalDeviceDynamicRenderingFeaturesKHR>();
    dynamicRenderingFeaturesKhr.dynamicRendering = true;

    indexingFeatures.runtimeDescriptorArray = true;
    indexingFeatures.descriptorBindingPartiallyBound = true;

    auto& createInfo = structureChain.get<vk::DeviceCreateInfo>();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = nullptr;
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
    std::array<vk::DescriptorPoolSize, 5> poolSizes = {
        vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_RESOURCES },
        vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_RESOURCES },
        vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_RESOURCES },
        vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_RESOURCES },
        vk::DescriptorPoolSize { vk::DescriptorType::eStorageBuffer, 1 },
    };

    vk::DescriptorPoolCreateInfo poolCreateInfo {};
    poolCreateInfo.flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
    poolCreateInfo.maxSets = MAX_BINDLESS_RESOURCES * poolSizes.size();
    poolCreateInfo.poolSizeCount = poolSizes.size();
    poolCreateInfo.pPoolSizes = poolSizes.data();
    util::VK_ASSERT(device.createDescriptorPool(&poolCreateInfo, nullptr, &bindlessPool), "Failed creating bindless pool!");

    std::array<vk::DescriptorSetLayoutBinding, 5> bindings;
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

    vk::DescriptorSetLayoutBinding& shadowBinding = bindings[3];
    shadowBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    shadowBinding.descriptorCount = MAX_BINDLESS_RESOURCES;
    shadowBinding.binding = static_cast<uint32_t>(BindlessBinding::eShadowmap);
    shadowBinding.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;

    vk::DescriptorSetLayoutBinding& materialBinding = bindings[4];
    materialBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
    materialBinding.descriptorCount = 1;
    materialBinding.binding = static_cast<uint32_t>(BindlessBinding::eMaterial);
    materialBinding.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;

    vk::StructureChain<vk::DescriptorSetLayoutCreateInfo, vk::DescriptorSetLayoutBindingFlagsCreateInfo> structureChain;

    auto& layoutCreateInfo = structureChain.get<vk::DescriptorSetLayoutCreateInfo>();
    layoutCreateInfo.bindingCount = bindings.size();
    layoutCreateInfo.pBindings = bindings.data();
    layoutCreateInfo.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;

    std::array<vk::DescriptorBindingFlagsEXT, bindings.size()> bindingFlags = {
        vk::DescriptorBindingFlagBits::ePartiallyBound,
        vk::DescriptorBindingFlagBits::ePartiallyBound,
        vk::DescriptorBindingFlagBits::ePartiallyBound,
        vk::DescriptorBindingFlagBits::ePartiallyBound,
        vk::DescriptorBindingFlagBits::ePartiallyBound
    };

    auto& extInfo = structureChain.get<vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT>();
    extInfo.bindingCount = bindings.size();
    extInfo.pBindingFlags = bindingFlags.data();

    util::VK_ASSERT(device.createDescriptorSetLayout(&layoutCreateInfo, nullptr, &bindlessLayout),
        "Failed creating bindless descriptor set layout.");

    vk::DescriptorSetAllocateInfo allocInfo {};
    allocInfo.descriptorPool = bindlessPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &bindlessLayout;

    util::VK_ASSERT(device.allocateDescriptorSets(&allocInfo, &bindlessSet), "Failed creating bindless descriptor set!");
}

void VulkanBrain::CreateBindlessMaterialBuffer()
{
    BufferCreation creation{};
    creation.SetSize(MAX_BINDLESS_RESOURCES * sizeof(Material::GPUInfo))
        .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer)
        .SetName("Bindless material uniform buffer");

    _bindlessMaterialBuffer = _bufferResourceManager.Create(creation);
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
