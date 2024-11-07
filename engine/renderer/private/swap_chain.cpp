#include "swap_chain.hpp"
#include "old_engine.hpp"
#include "vulkan/vulkan.h"
#include "vulkan_helper.hpp"

SwapChain::SwapChain(const VulkanContext& brain, const glm::uvec2& screenSize)
    : _brain(brain)
{
    CreateSwapChain(screenSize);
}

SwapChain::~SwapChain()
{
    CleanUpSwapChain();
}

void SwapChain::CreateSwapChain(const glm::uvec2& screenSize)
{
    _imageSize = screenSize;
    SupportDetails swapChainSupport = QuerySupport(_brain.physicalDevice, _brain.surface);

    auto surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    auto presentMode = ChoosePresentMode(swapChainSupport.presentModes);
    auto extent = ChooseSwapExtent(swapChainSupport.capabilities, screenSize);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    vk::SwapchainCreateInfoKHR createInfo {};
    createInfo.surface = _brain.surface;
    createInfo.minImageCount = imageCount + 1;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment; // TODO: Can change this later to a memory transfer operation, when doing post-processing.
    if (swapChainSupport.capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferSrc)
        createInfo.imageUsage |= vk::ImageUsageFlagBits::eTransferSrc;
    if (swapChainSupport.capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst)
        createInfo.imageUsage |= vk::ImageUsageFlagBits::eTransferDst;

    uint32_t queueFamilyIndices[] = { _brain.queueFamilyIndices.graphicsFamily.value(), _brain.queueFamilyIndices.presentFamily.value() };
    if (_brain.queueFamilyIndices.graphicsFamily != _brain.queueFamilyIndices.presentFamily)
    {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = vk::True;
    createInfo.oldSwapchain = nullptr;

    util::VK_ASSERT(_brain.device.createSwapchainKHR(&createInfo, nullptr, &_swapChain), "Failed creating swap chain!");

    util::NameObject(_swapChain, "Main Swapchain", _brain);

    _images = _brain.device.getSwapchainImagesKHR(_swapChain);
    _format = surfaceFormat.format;
    _extent = extent;

    CreateSwapChainImageViews();
}

void SwapChain::Resize(const glm::uvec2& screenSize)
{
    _brain.device.waitIdle();

    CleanUpSwapChain();

    CreateSwapChain(screenSize);
}

void SwapChain::CreateSwapChainImageViews()
{
    _imageViews.resize(_images.size());
    for (size_t i = 0; i < _imageViews.size(); ++i)
    {
        vk::ImageViewCreateInfo createInfo {
            .flags = vk::ImageViewCreateFlags {},
            .image = _images[i],
            .viewType = vk::ImageViewType::e2D,
            .format = _format,
            .components = vk::ComponentMapping { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity },
            .subresourceRange = vk::ImageSubresourceRange {
                vk::ImageAspectFlagBits::eColor, // aspect mask
                0, // base mip level
                1, // level count
                0, // base array level
                1 // layer count
            }
        };
        util::VK_ASSERT(_brain.device.createImageView(&createInfo, nullptr, &_imageViews[i]), "Failed creating image view for swap chain!");

        util::NameObject(_imageViews[i], "Swapchain Image View", _brain);
        util::NameObject(_images[i], "Swapchain Image", _brain);
    }
}

SwapChain::SupportDetails SwapChain::QuerySupport(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    SupportDetails details {};

    util::VK_ASSERT(device.getSurfaceCapabilitiesKHR(surface, &details.capabilities), "Failed getting surface capabilities from physical device!");

    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);

    return details;
}

vk::SurfaceFormatKHR SwapChain::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
    for (const auto& format : availableFormats)
    {
        if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return format;
    }

    return availableFormats[0];
}

vk::PresentModeKHR SwapChain::ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    auto it = std::find_if(availablePresentModes.begin(), availablePresentModes.end(),
        [](const auto& mode)
        { return mode == vk::PresentModeKHR::eMailbox; });
    if (it != availablePresentModes.end())
        return *it;

    it = std::find_if(availablePresentModes.begin(), availablePresentModes.end(),
        [](const auto& mode)
        { return mode == vk::PresentModeKHR::eFifo; });
    if (it != availablePresentModes.end())
        return *it;

    return availablePresentModes[0];
}

vk::Extent2D SwapChain::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, const glm::uvec2& screenSize)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    vk::Extent2D extent = { screenSize.x, screenSize.y };
    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return extent;
}

void SwapChain::CleanUpSwapChain()
{
    for (auto imageView : _imageViews)
        _brain.device.destroy(imageView);

    _brain.device.destroy(_swapChain);
}
