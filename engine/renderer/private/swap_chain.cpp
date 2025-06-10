#include "swap_chain.hpp"

#include "graphics_context.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

SwapChain::SwapChain(const std::shared_ptr<GraphicsContext>& context, const glm::uvec2& screenSize)
    : _context(context)
{
    CreateSwapChain(screenSize);
}

SwapChain::~SwapChain()
{
    CleanUpSwapChain();
}

void SwapChain::CreateSwapChain(const glm::uvec2& screenSize)
{
    auto vkContext { _context->VulkanContext() };

    _imageSize = screenSize;
    SupportDetails swapChainSupport = QuerySupport(vkContext->PhysicalDevice(), vkContext->Surface());

    auto surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    auto extent = ChooseSwapExtent(swapChainSupport.capabilities, screenSize);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    vk::SwapchainCreateInfoKHR createInfo {};
    createInfo.surface = vkContext->Surface();
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

    uint32_t queueFamilyIndices[] = { vkContext->QueueFamilies().graphicsFamily.value(), vkContext->QueueFamilies().presentFamily.value() };
    if (vkContext->QueueFamilies().graphicsFamily != vkContext->QueueFamilies().presentFamily)
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
    createInfo.presentMode = _presentMode;
    createInfo.clipped = vk::True;
    createInfo.oldSwapchain = nullptr;

    util::VK_ASSERT(vkContext->Device().createSwapchainKHR(&createInfo, nullptr, &_swapChain), "Failed creating swap chain!");

    util::NameObject(_swapChain, "Main Swapchain", vkContext);

    _images = vkContext->Device().getSwapchainImagesKHR(_swapChain);
    _format = surfaceFormat.format;
    _extent = extent;

    CreateSwapChainImageViews();
}

void SwapChain::Resize(const glm::uvec2& screenSize)
{
    auto vkContext { _context->VulkanContext() };

    vkContext->Device().waitIdle();

    CleanUpSwapChain();

    CreateSwapChain(screenSize);
}

void SwapChain::CreateSwapChainImageViews()
{
    auto vkContext { _context->VulkanContext() };

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
        util::VK_ASSERT(vkContext->Device().createImageView(&createInfo, nullptr, &_imageViews[i]), "Failed creating image view for swap chain!");

        util::NameObject(_imageViews[i], "Swapchain Image View", vkContext);
        util::NameObject(_images[i], "Swapchain Image", vkContext);
    }
}

bool SwapChain::SetPresentMode(vk::PresentModeKHR presentMode)
{
    SupportDetails swapChainSupport = QuerySupport(_context->VulkanContext()->PhysicalDevice(), _context->VulkanContext()->Surface());
    const auto it = std::find(swapChainSupport.presentModes.begin(), swapChainSupport.presentModes.end(), presentMode);

    if (_presentMode == *it)
    {
        return false;
    }

    if (it != swapChainSupport.presentModes.end())
    {
        _presentMode = *it;
    }
    else
    {
        _presentMode = vk::PresentModeKHR::eFifo;
    }

    return true;
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
    auto vkContext { _context->VulkanContext() };

    for (auto imageView : _imageViews)
    {
        vkContext->Device().destroy(imageView);
    }

    vkContext->Device().destroy(_swapChain);
}
